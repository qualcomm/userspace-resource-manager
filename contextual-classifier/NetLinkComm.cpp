// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "NetLinkComm.h"
#include "ContextualClassifier.h"
#include "Logger.h"
#include <cerrno>
#include <cstdarg>
#include <cstring>
#include <unistd.h>

#define CLASSIFIER_TAG "NetLinkComm"

static std::string format_string(const char *fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    return std::string(buffer);
}

NetLinkComm::NetLinkComm() : nl_sock(-1) {}

NetLinkComm::~NetLinkComm() { close_socket(); }

void NetLinkComm::close_socket() {
    if (nl_sock != -1) {
        close(nl_sock);
        nl_sock = -1;
    }
}

int NetLinkComm::get_socket() const { return nl_sock; }

int NetLinkComm::connect() {
    int rc;
    struct sockaddr_nl sa_nl;

    nl_sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
    if (nl_sock == -1) {
        LOGE(CLASSIFIER_TAG, format_string("socket: %s", strerror(errno)));
        return -1;
    }

    sa_nl.nl_family = AF_NETLINK;
    sa_nl.nl_groups = CN_IDX_PROC;
    sa_nl.nl_pid = getpid();

    rc = bind(nl_sock, (struct sockaddr *)&sa_nl, sizeof(sa_nl));
    if (rc == -1) {
        LOGE(CLASSIFIER_TAG, format_string("bind: %s", strerror(errno)));
        close_socket();
        return -1;
    }

    return nl_sock;
}

int NetLinkComm::set_listen(bool enable) {
    int rc;
    struct __attribute__((aligned(NLMSG_ALIGNTO))) {
        struct nlmsghdr nl_hdr;
        struct __attribute__((__packed__)) {
            struct cn_msg_hdr cn_msg;
            enum proc_cn_mcast_op cn_mcast;
        };
    } nlcn_msg;

    memset(&nlcn_msg, 0, sizeof(nlcn_msg));
    nlcn_msg.nl_hdr.nlmsg_len = sizeof(nlcn_msg);
    nlcn_msg.nl_hdr.nlmsg_pid = getpid();
    nlcn_msg.nl_hdr.nlmsg_type = NLMSG_DONE;

    nlcn_msg.cn_msg.id.idx = CN_IDX_PROC;
    nlcn_msg.cn_msg.id.val = CN_VAL_PROC;
    nlcn_msg.cn_msg.len = sizeof(enum proc_cn_mcast_op);

    nlcn_msg.cn_mcast = enable ? PROC_CN_MCAST_LISTEN : PROC_CN_MCAST_IGNORE;

    rc = send(nl_sock, &nlcn_msg, sizeof(nlcn_msg), 0);
    if (rc == -1) {
        LOGE(CLASSIFIER_TAG,
             format_string("netlink send: %s", strerror(errno)));
        return -1;
    }

    return 0;
}

int NetLinkComm::RecvEvent(ProcEvent &ev) {
    int rc = 0;
    struct __attribute__((aligned(NLMSG_ALIGNTO))) {
        struct nlmsghdr nl_hdr;
        struct __attribute__((__packed__)) {
            struct cn_msg_hdr cn_msg;
            struct proc_event proc_ev;
        };
    } nlcn_msg;

    rc = recv(nl_sock, &nlcn_msg, sizeof(nlcn_msg), 0);
    if (rc == 0) {
        // Socket shutdown or no more data.
        return 0;
    }
    if (rc == -1) {
        if (errno == EINTR) {
            // Caller (ContextualClassifier::HandleProcEv) will handle EINTR.
            return -1;
        }
        LOGE(CLASSIFIER_TAG,
             format_string("netlink recv: %s", strerror(errno)));
        return -1;
    }

    ev.pid = -1;
    ev.tgid = -1;
    ev.type = CC_IGNORE;

    switch (nlcn_msg.proc_ev.what) {
    case proc_event::PROC_EVENT_NONE:
        // No actionable event.
        break;
    case proc_event::PROC_EVENT_FORK:
        LOGD(CLASSIFIER_TAG,
             format_string("fork: parent tid=%d pid=%d -> child tid=%d pid=%d",
                           nlcn_msg.proc_ev.event_data.fork.parent_pid,
                           nlcn_msg.proc_ev.event_data.fork.parent_tgid,
                           nlcn_msg.proc_ev.event_data.fork.child_pid,
                           nlcn_msg.proc_ev.event_data.fork.child_tgid));
        break;
    case proc_event::PROC_EVENT_EXEC:
        LOGD(CLASSIFIER_TAG,
             format_string("Received PROC_EVENT_EXEC for tid=%d pid=%d",
                           nlcn_msg.proc_ev.event_data.exec.process_pid,
                           nlcn_msg.proc_ev.event_data.exec.process_tgid));
        ev.pid = nlcn_msg.proc_ev.event_data.exec.process_pid;
        ev.tgid = nlcn_msg.proc_ev.event_data.exec.process_tgid;
        ev.type = CC_APP_OPEN;
        rc = CC_APP_OPEN;
        break;
    case proc_event::PROC_EVENT_UID:
        LOGD(CLASSIFIER_TAG,
             format_string("uid change: tid=%d pid=%d from %d to %d",
                           nlcn_msg.proc_ev.event_data.id.process_pid,
                           nlcn_msg.proc_ev.event_data.id.process_tgid,
                           nlcn_msg.proc_ev.event_data.id.r.ruid,
                           nlcn_msg.proc_ev.event_data.id.e.euid));
        break;
    case proc_event::PROC_EVENT_GID:
        LOGD(CLASSIFIER_TAG,
             format_string("gid change: tid=%d pid=%d from %d to %d",
                           nlcn_msg.proc_ev.event_data.id.process_pid,
                           nlcn_msg.proc_ev.event_data.id.process_tgid,
                           nlcn_msg.proc_ev.event_data.id.r.rgid,
                           nlcn_msg.proc_ev.event_data.id.e.egid));
        break;
    case proc_event::PROC_EVENT_EXIT:
        LOGD(CLASSIFIER_TAG,
             format_string("exit: tid=%d pid=%d exit_code=%d",
                           nlcn_msg.proc_ev.event_data.exit.process_pid,
                           nlcn_msg.proc_ev.event_data.exit.process_tgid,
                           nlcn_msg.proc_ev.event_data.exit.exit_code));
        ev.pid = nlcn_msg.proc_ev.event_data.exit.process_pid;
        ev.tgid = nlcn_msg.proc_ev.event_data.exit.process_tgid;
        ev.type = CC_APP_CLOSE;
        rc = CC_APP_CLOSE;
        break;
    default:
        LOGW(CLASSIFIER_TAG, "unhandled proc event");
        break;
    }

    return rc;
}
