# Userspace Resource Manager: A System Resource Tuning Framework

## Table of Contents


- [1. Overview](#1-overview)
  - [1.1. URM Features](#11-urm-features)
  - [1.2. Why URM is Needed](#12-why-urm-is-needed)
  - [1.3. What URM offers](#13-what-urm-offers)
    - [1.3.1 Provisioning System Resources](#131-provisioning-system-resources)
    - [1.3.2 Use-case/Workload/Scenario Tuning](#132-use-case-workload-scenario-tuning)
    - [1.3.3 Per-App Tuning](#133-per-app-tuning)
  - [1.4. Extension Framework](#14-extension-framework)

- [2. Getting Started](#2-getting-started)

- [3. Configurable Resources and Signals](#3-configurable-resources-and-signals)
  - [3.1. Resources](#31-resources)
  - [3.2. Signals](#32-signals)

- [4. URM Interface](#4-urm-interface)
  - [4.1. Platform Abstraction Layer](#41-platform-abstraction-layer)
  - [4.2. URM APIs](#42-urm-apis)
    - [4.2.1. tuneResources](#421-tuneresources)
      - [4.2.1.1. SysResource](#4211-sysresource)
      - [4.2.1.2. Resource ResCode Construction](#4212-resource-rescode-construction)
      - [4.2.1.3. Resource ResInfo Construction](#4213-resource-resinfo-construction)
      - [4.2.1.4. Example](#4214-example)
    - [4.2.2. retuneResources](#422-retuneresources)
      - [4.2.2.1. Example](#4221-example)
    - [4.2.3. untuneResources](#423-untuneresources)
      - [4.2.3.1. Example](#4231-example)
    - [4.2.4. tuneSignal](#424-tunesignal)
      - [4.2.4.1. Signal Code Construction](#4241-signal-code-construction)
    - [4.2.5. untuneSignal](#425-untunesignal)
    - [4.2.6. relaySignal](#426-relaysignal)
    - [4.2.7. getProp](#427-getprop)
  - [4.3. Configs](#43-configs)
    - [4.3.1. Initialization Configs](#431-initialization-configs)
    - [4.3.2. Resource Configs](#432-resource-configs)
    - [4.3.3. Properties Config](#433-properties-config)
    - [4.3.4. Signal Configs](#434-signal-configs)
    - [4.3.5. Per-App Configs](#435-per-app-configs)
  - [4.4. Urm Config Parsing](#44-urm-config-parsing)

- [5. Client CLI](#5-client-cli)
  - [5.1. Send a Tune Request](#51-send-a-tune-request)
  - [5.2. Send an Untune Request](#52-send-an-untune-request)
  - [5.3. Send a Retune Request](#53-send-a-retune-request)
  - [5.4. Send a getProp Request](#54-send-a-getprop-request)
  - [5.5. Send a tuneSignal Request](#55-send-a-tunesignal-request)

- [6. Customizations & Extensions](#6-customizations--extensions)
  - [6.1. Extensions Interface](#61-extensions-interface)
    - [6.1.1. Registering Resource Callbacks](#611-registering-resource-callbacks)
    - [6.1.2. Registering Custom Configs](#612-registering-custom-configs)
  - [6.2. Custom Configs](#62-custom-configs)
    - [6.2.1. Initialization Configs](#621-initialization-configs)
    - [6.2.2. Resource Configs](#622-resource-configs)
    - [6.2.3. Properties Config](#623-properties-config)
    - [6.2.4. Signal Configs](#624-signal-configs)
    - [6.2.5. Target Configs](#625-target-configs)
  - [6.3. Extension Features](#63-extension-features)

- [7. URM Implementation Details](#7-urm-implementation-details)
  - [7.1. Project Structure](#71-project-structure)
  - [7.2. Userspace Resource Manager Key Points](#72-userspace-resource-manager-key-points)
    - [7.2.1. Contextual Classifier](#721-contextual-classifier)
    - [7.2.2. Resource Tuner](#722-resource-tuner)
    - [7.2.3. URM Initialization](#723-urm-initialization)
    - [7.2.4. Request Processing](#724-request-processing)
  - [7.3. Client-Level Permissions](#73-client-level-permissions)
  - [7.4. Resource-Level Policies](#74-resource-level-policies)
  - [7.5. Request-Level Priorities](#75-request-level-priorities)
  - [7.6. Pulse Monitor](#76-pulse-monitor)
  - [7.7. Rate Limiter](#77-rate-limiter)
  - [7.8. Duplicate Checking](#78-duplicate-checking)
  - [7.9. Dynamic Mapper](#79-dynamic-mapper)
  - [7.10. Display-Aware Operational Modes](#710-display-aware-operational-modes)
  - [7.11. Crash Recovery](#711-crash-recovery)
  - [7.12. Flexible Packaging](#712-flexible-packaging)
  - [7.13. Pre-Allocate Capacity for efficiency](#713-pre-allocate-capacity-for-efficiency)

- [8. Contact](#8-contact)

- [9. License](#9-license)

# 1. Overview
Userspace Resource Manager (URM) is a lightweight, extensible framework designed to intelligently manage and provision system resources from userspace. It brings together two key components that enable dynamic, context‑aware optimization of performance and power across diverse embedded and resource‑constrained environments:
- Contextual Classifier
A Floret‑based classifier that identifies static workload contexts—such as camera pipelines, multimedia sessions, installations, or background services. By understanding what the system is running, URM can apply the right resource strategy without manual intervention.
- Resource Tuner
A userspace engine that dynamically provisions system resources—CPU, memory, GPU, I/O, caches, and more—using standard kernel interfaces such as procfs, sysfs, and cgroups. This enables fine‑grained control of system operating points to balance performance and power for each application or use case.
```
                +---------------------------+
                | Process Event Listener    |
                |       (NetLinkComm)       |
                +-------------+-------------+
                              |                (Event trigger)
                              |             (from app using APIs)
                              v                      |
                +---------------------------+        |
      +---------|    Contextual Classifier  |        |
      |         +-------------+-------------+        |
      |                       |                      |
      |                   (Workload)                 |
      |                       |                      |
      v                       v                      |
 +----+----+    +---------------------------+        |
 |  Yaml   |<---|     Resource Tuner        |<-------+
 | Configs |    +-------------+-------------+
 +---------+                  |
                              v
                       +--------------+
                       |    Action    |
                       +--------------+
```
### 1.1. URM Features
- Usecase detection
- System respource provisioning
- Usecase/workload tuning
- Perapp tuning support
- Concurrent requests support
- Multi-target support
- Auto package selection (overlays support, ex. picking up adreno resources over freedrino when available)
- Yaml Configs & target specific yaml configs
  - Init configs
  - Resource configs
  - Signal configs
  - Perapp config
  - Target config (optional, can be specified when want to override urm auto-detected configuration)
- Custom yaml configs
- Customized resources
- Extensions (Customers, developers can develop plugins for their features)
- Cgroups support
- Mpam support
- Security frameworks support (currently sepolicies)
- Rate Limiter (avoids flooding of requests, tracked client-wise)
- Action Thresholds
- Authorizer (System, 3rd party requests differentiation)
- Request Priorities (Support for high, low priorities)
- Pulse Monitor (Removes dead client requests)

### 1.2. Why URM is Needed
Modern workloads vary drastically across segments like servers, compute, XR, mobile, and IoT. Each use case exhibits unique characteristics. Some may demand high CPU frequency, others may require sustained GPU throughput, and many hinge on efficient caching or more memory bandwidth.

At the same time, these workloads run on a broad spectrum of hardware platforms with different capabilities, power envelopes, and user expectations. A "one‑size‑fits‑all" tuning approach fails in such environments.

URM addresses this challenge by offering:
- Per‑use‑case adaptability
- Hardware‑aware tuning
- User‑experience‑driven power/perf balancing

This allows systems to achieve consistent performance while minimizing energy consumption.

### 1.3. What URM offers

#### 1.3.1 Provisioning System Resources
URM help in modifying system behavior to manage intermittent workloads. For example, if a
specific code segment must run at a higher CPU frequency for a certain duration, use URM APIs
within that code to boost the CPU frequency. URM efficiently handles concurrent resource requests from multiple clients. When several requests are aimed at the same resource, URM aggregates them to achieve the optimal performance level needed by the device. When a client is no longer active, URM releases all the tuneresource requests associated with that client.

URM offers tuneResources, untuneResources, retuneResources APIs for provisioning system resources. Full details of these APIs and their usage examples were given in [URM Interface](#4-urm-interface)

Here are few examples of how resources can be used for influencing system behavior
- Moving tasks to silver cluster
![affinity](Figures/task_affinity.png "tasks affinity")
Resources:
RES_CGRP_RUN_CORES, RES_CGRP_MOVE_PID

- Running tasks exclusively on a cluster
![exclusivity](Figures/run_exclusively.png "tasks exclusivity")
Resources:
RES_CGRP_RUN_CORES_EXCL, RES_CGRP_MOVE_PID

- Reducing CPU Share to 50%
![cpushare](Figures/cpu_share.png "cpu share")
Resources:
RES_CGRP_LIMIT_CPU_TIME, RES_CGRP_MOVE_PID

- Marking low-latency threads
![lowlatency](Figures/low_latency.png "low latency")
Resources:
RES_CGRP_CPU_LATENCY, RES_CGRP_MOVE_PID

- Utilclamp
![utilclamp](Figures/utilclamp.png "utilclamp")
Resources:
RES_CGRP_UCLAMP_MIN, RES_CGRP_MOVE_PID

- LPM
![lpm](Figures/lpm.png "lpm")
Resources:
(RES_PM_QOS_LATENCY, CLUSTER_BIG_ALL_CORES)

#### 1.3.2 Use-case/Workload/Scenario Tuning
URM uses Signals, a mechanism that adjusts system resources in real time based on events such as:
- App launches
- On‑device workflows or triggers

These behaviors are governed by flexible, human‑readable YAML configurations, making policies easy to author, review, and evolve.

URM detects an app open and sends a URM_OPEN signal, applies tuning in yaml config if available. URM also offers signal APIs (tuneSignal, untuneSignal) which can be called from within code segments of application or framework to address the needs of any intermittent workload, scenario to get better power and perforamnce trade-off.

#### 1.3.3 Per-App tuning
URM helps to tune apps, tuning can be given in yaml config file and applies this tuning when an app is opened. Developers, customers can tune their apps as per their requirement. Per-app configs were explained in detail in [Per-App Configs](#435-per-app-configs) section.

### 1.4. Extension Framework
URM also provides extension APIs that allow other modules or applications to:

- Register custom resources
- Define custom provisioning logic
- Add Segment‑specific or product‑specific behaviors

This ensures the framework remains adaptable as hardware, use cases, and business requirements evolve.


<div style="page-break-after: always;"></div>

# 2. Getting Started

To get started with the project:
[Build and install](../README.md#build-and-install-instructions)

# 3. Configurable Resources and Signals

## 3.1. Resources

**Resource Code** is composed of two least significant bytes (the lower 16 bits) contains resource ID and next significant byte contains resource type
i.e. 0xrr tt iiii  (i: resource id, t: resource type, r: reserved)

The following resource codes are supported resources or operations on upstream Linux. Target or segment specific resources can be defined further. These resources can be directly used in client-side code or yaml configs and provide an easy and standard way to specify a certain resource, without needing to formulate the opcodes.

|      Resource Name                  |         Id        |
|-------------------------------------|-------------------|
|   RES_CPU_DMA_LATENCY               |   0x 00 01 0000   |
|   RES_PM_QOS_LATENCY                |   0x 00 01 0001   |
|   RES_CPU_IDLE_DISABLE_ST0          |   0x 00 01 0002   |
|   RES_CPU_IDLE_DISABLE_ST1          |   0x 00 01 0003   |
|   RES_CPU_IDLE_DISABLE_ST2          |   0x 00 01 0004   |
|   RES_SCHED_UTIL_CLAMP_MIN          |   0x 00 03 0000   |
|   RES_SCHED_UTIL_CLAMP_MAX          |   0x 00 03 0001   |
|   RES_SCHED_ENERGY_AWARE            |   0x 00 03 0002   |
|   RES_SCHED_RT_RUNTIME              |   0x 00 03 0003   |
|   RES_PER_TASK_AFFINITY             |   0x 00 03 0004   |
|   RES_SCALE_MIN_FREQ                |   0x 00 04 0000   |
|   RES_SCALE_MAX_FREQ                |   0x 00 04 0001   |
|   RES_RATE_LIMIT_US                 |   0x 00 04 0002   |
|   RES_DEVFREQ_GPU_MAX               |   0x 00 05 0000   |
|   RES_DEVFREQ_GPU_MIN               |   0x 00 05 0001   |
|   RES_DEVFREQ_GPU_POLL_INTV         |   0x 00 05 0002   |
|   RES_CGRP_MOVE_PID                 |   0x 00 09 0000   |
|   RES_CGRP_MOVE_TID                 |   0x 00 09 0001   |
|   RES_CGRP_RUN_CORES                |   0x 00 09 0002   |
|   RES_CGRP_RUN_CORES_EXCL           |   0x 00 09 0003   |
|   RES_CGRP_FREEZE                   |   0x 00 09 0004   |
|   RES_CGRP_LIMIT_CPU_TIME           |   0x 00 09 0005   |
|   RES_CGRP_RUN_WHEN_CPU_IDLE        |   0x 00 09 0006   |
|   RES_CGRP_UCLAMP_MIN               |   0x 00 09 0007   |
|   RES_CGRP_UCLAMP_MAX               |   0x 00 09 0008   |
|   RES_CGRP_REL_CPU_WEIGHT           |   0x 00 09 0009   |
|   RES_CGRP_HIGH_MEM                 |   0x 00 09 000a   |
|   RES_CGRP_MAX_MEM                  |   0x 00 09 000b   |
|   RES_CGRP_LOW_MEM                  |   0x 00 09 000c   |
|   RES_CGRP_MIN_MEM                  |   0x 00 09 000d   |
|   RES_CGRP_SWAP_MAX_MEMORY          |   0x 00 09 000e   |
|   RES_CGRP_IO_WEIGHT                |   0x 00 09 000f   |
|   RES_CGRP_CPU_LATENCY              |   0x 00 09 0011   |
|   RES_DEVFREQ_UFS_MAX               |   0x 00 0a 0000   |
|   RES_DEVFREQ_UFS_MIN               |   0x 00 0a 0001   |
|   RES_DEVFREQ_UFS_POLL_INTV         |   0x 00 0a 0002   |
|   RES_IRQ_AFFINE                    |   0x 00 0b 0000   |
|   RES_DEF_IRQ_AFFINE                |   0x 00 0b 0001   |


The above mentioned list of enums are available in the interface file "UrmPlatformAL.h".

## 3.2. Signals

**Signal Code** is composed of two least significant bytes (the lower 16 bits) contains signal ID and next significant byte contains signal category
i.e. 0xrr cc iiii  (i: signal id, t: signal category, r: reserved)

The following signal codes are supported signals. Target or segment specific custom signals can be defined further.

|       Signal Code             |  Code           |
|-------------------------------|-----------------|
|   URM_SIG_APP_OPEN            | 0x 00 02 0001   |
|   URM_SIG_BROWSER_APP_OPEN    | 0x 00 02 0002   |
|   URM_SIG_GAME_APP_OPEN       | 0x 00 02 0003   |
|   URM_SIG_MULTIMEDIA_APP_OPEN | 0x 00 02 0004   |

The above mentioned list of enums are available in the interface file "UrmPlatformAL.h".

# 4. URM Interface

   - Platform Abstraction Layer
   - URM APIs
   - Configs

## 4.1. Platform Abstraction Layer

This section defines logical layer to improve code and config portability across different targets and segments. 

Some logical maps are present in init config, these can not be changed.

**Logical Cluster Map**
Logical IDs for clusters. Configs of cluster map can be found in InitConfigs->ClusterMap section
| LgcId  |     Name   |
|--------|------------|
|   0    |   "little" |
|   1    |   "big"    |
|   2    |   "prime"  |

**Cgroups map**
Logical IDs for cgroups. Configs of cgroups map in InitConfigs->CgroupsInfo section

| Lgc Cgrp No |   Cgrp Name      |
|-------------|------------------|
|      0      |  "root"          |
|      1      |  "init.scope"    |
|      2      |  "system.slice"  |
|      3      |  "user.slice"    |
|      4      |  "focused.slice" |

**Mpam Groups Map**
Logical IDs for MPAM groups. Configs of MPAM group map in InitConfigs->MpamGroupsInfo section
| LgcId  |    Mpam grp Name  |
|--------|-------------------|
|   0    |      "default"    |
|   1    |       "video"     |
|   2    |       "camera"    |
|   3    |       "games"     |


**Resource Types**  These are defined in UrmPlatformAL.h
|Resource Type| type  |Resource Type| type  |
|-------------|-------|-------------|-------|
| Cpu Lpm     |   0   | Memory Qos  |  7    |
| Cache Mgmt  |   1   | Mpam Qos    |  8    |
| Cpu Sched   |   3   | Cgroup Ops  |  9    | 
| Cpu Freq    |   4   | Storage IO  |  10   |
| Gpu Opp     |   5   | Custom      |  128+ |
| Npu         |   6   |             |       |

**Signal Categories** These are defined UrmPlatformAL.h
|    Signal Category       |  Value          |
|--------------------------|-----------------|
|   URM_SIG_CAT_TEST       |       1         |
|   URM_SIG_CAT_GENERIC    |       2         |
|   URM_SIG_CAT_MULTIMEDIA |       3         |
|   URM_SIG_CAT_GAMING     |       4         |
|   URM_SIG_CAT_BROWSER    |       5         |
|   URM_SIG_CAT_CUSTOM     |      128+       |

Custom resource types and signal categories must be >=128 to avoid conflicts with default types.

## 4.2. URM APIs
This API suite allows you to manage system resource provisioning through tuning requests. You can issue, modify, or withdraw resource tuning requests with specified durations and priorities.

APIs examples: 
https://github.com/qualcomm/userspace-resource-manager/tree/main/docs/Examples

### 4.2.1. tuneResources

**Description:**
Issues a resource provisioning (or tuning) request for a finite or infinite duration.

**API Signature:**
```cpp
int64_t tuneResources(int64_t duration,
                      int32_t prop,
                      int32_t numRes,
                      SysResource* resourceList);

```

**Parameters:**

- `duration` (`int64_t`): Duration in milliseconds for which the Resource(s) should be Provisioned. Use `-1` for an infinite duration.

- `properties` (`int32_t`): Properties of the Request.
                            - The last 8 bits [25 - 32] store the Request Priority (HIGH / LOW)
                            - The Next 8 bits [17 - 24] represent a Boolean Flag, which indicates
                              if the Request should be processed in the background (in case of Display Off or Doze Mode).

- `numRes` (`int32_t`): Number of resources to be tuned as part of the Request.

- `resourceList` (`SysResource*`): List of Resources to be provisioned as part of the Request.

**Returns:**
`int64_t`
- **A positive unique handle** identifying the issued request (used for future `retune` or `untune` operations)
- `-1` otherwise.

#### 4.2.1.1. SysResource

```cpp
typedef struct {
    uint32_t mResCode;
    int32_t mResInfo;
    int32_t mOptionalInfo;
    int32_t mNumValues;

    union {
        int32_t value;
        int32_t* values;
    } mResValue;
} SysResource;
```

`mResCode` (`uint32_t`): An unsigned 32-bit unique identifier for the resource. It encodes essential information that is useful in abstracting away the system specific details.

`mResInfo`  (`int32_t`): Encodes operation-specific information such as the Logical cluster and Logical core ID, and MPAM part ID.

`mOptionalInfo`  (`int32_t`): Additional optional metadata, useful for custom or extended resource configurations.

`mNumValues`  (`int32_t`): Number of values associated with the resource. If multiple values are needed, this must be set accordingly.

`Value / Values`  (`uint32_t *`): It is a single value when the resource requires a single value or a pointer to an array of values for multi-value configurations.

#### 4.2.1.2. Resource ResCode Construction
This section describes how this code can be constructed. URM implements a platform abstraction layer (logical layer) which provides a transparent and consistent way for indexing resources. This makes it easy for the clients to identify the resource they want to provision, without needing to worry about portability issues across targets.

```text
Resource code (unsigned 32 bit) is composed of two fields:
- ResID (LSBs 0-15)
- ResType (next 8 bits, 16-23)

31                 24                 16                         0
+--------------------+------------------+------------------------+
| Reserved (1 byte)  | ResType (1 byte) |     ResId (2 bytes)    |
+--------------------+------------------+------------------------+
```

These fields combined together can uniquely identify a resource across targets, hence making the code operating on these resources interchangable.

```text
Examples:
Code for cpu_freq (0x04) resource type and min_freq (0x00) operation : 0x00040000

                     cpu_freq  min_freq
0x00040000 - 0x00    04        00000

Code for cgrp (0x09) resource type and run-cores (0x02) operation : 0x00090002

                     cgrp      run-on-cores
0x00090002 - 0x00    09        00002
```

The macro **CONSTRUCT_RES_CODE** can be used for generating opcodes directly from the resource type and resource id:
```cpp
   uint32_t resCode = CONSTRUCT_RES_CODE(0x09, 0x02);
```

#### 4.2.1.3. Resource ResInfo Construction

The ResInfo field specified as part of the SysResource struct encodes the following information:

```text
- Bits [0 - 7]: Core Information
- Bits [8 - 15]: Cluster Information
- Bits [16 - 31]: MPAM Application Information

31          24            16            8             0
+-------------+-------------+------------+------------+
|      MPAM Info            |  Cluster   |     Core   |
+-------------+-------------+------------+------------+
```

```text
Examples:
ResInfo for if you want operate resource on big cluster(0x01) : 0x00000100

                       cluster   core
0x00040000 - 0x0000    01        00
```

The macros SET_RESOURCE_CLUSTER_VALUE and SET_RESOURCE_CORE_VALUE are provided for clients to easily specify the configuration core / cluster information, if needed.

```cpp
int32_t mResInfo = 0;
// Set logical cluster ID to 1 (Gold)
mResInfo = SET_RESOURCE_CLUSTER_VALUE(mResInfo, 1);
```

```cpp
int32_t mResInfo = 0;
// 2nd core on the little cluster
mResInfo = SET_RESOURCE_CLUSTER_VALUE(mResInfo, 0);
mResInfo = SET_RESOURCE_CORE_VALUE(mResInfo, 2)
```
**Resource mOptionalInfo**
The mOptionalInfo field, as the name suggests can be used to store any optional information needed for resource application. It can be used as part of customized resource applier / tear callbacks if needed, else it can be left as 0.

**Resource mNumValues and value(s)**
URM Allows for both single and multi-valued configurations. The SysResource struct allows for storing all the configuration values. The field mNumValues specifies the number of values to be configured

The following example shows a simple single-valued config, use the "value" field in the mResValue union.

```cpp
    SysResource resourceList[1];
    memset(&resourceList[0], 0, sizeof(SysResource));
    resourceList[0].mResCode = 0x0002a0be;
    resourceList[0].mResInfo = SET_RESOURCE_CLUSTER_VALUE(resourceList[0].mResInfo, 1);
    resourceList[0].mNumValues = 1;
    resourceList[0].mResValue.value = 999;
```

If multi-valued config is needed, the use the "values" field in the mResValue union.

```cpp
    SysResource* resourceList = new SysResource[1];
    memset(&resourceList[0], 0, sizeof(SysResource));

    resourceList[0].mResCode = 0x00090007;
    resourceList[0].mResInfo = 0;
    resourceList[0].mNumValues = 2;
    resourceList[0].mResValue.values = new int32_t[resourceList[0].mNumValues];
    resourceList[0].mResValue.values[0] = 0;
    resourceList[0].mResValue.values[1] = 55;
```
The caller is responsible for freeing SysResource objects and any allocated value arrays.

#### 4.2.1.4. Example

```cpp
#include <iostream>
#include <Urm/UrmAPIs.h>

void sendRequest() {
    // Define resources

    // Need to tune a single resource
    SysResource* resourceList = new SysResource[2];

    //Seting cpu min_freq of big cluster
    resourceList[0].mResCode = 0x00040000;
    resourceList[0].mResInfo = SET_RESOURCE_CLUSTER_VALUE(resourceList[0].mResInfo, 1);
    resourceList[0].mNumValues = 1;
    resourceList[0].mResValue.value = 980;

    // Resource Identifier, capping cpu max_freq of prime cluster
    resourceList[1].mResCode = 0x00040001;
    resourceList[1].mResInfo = SET_RESOURCE_CLUSTER_VALUE(resourceList[1].mResInfo, 2);
    resourceList[1].mNumValues = 1;
    resourceList[1].mResValue.value = 1024;

    // Issue the Tune Request
    int64_t handle = tuneResources(5000, 0, 2, resourceList);

    if(handle < 0) {
        std::cerr<<"Failed to issue tuning request."<<std::endl;
    } else {
        std::cout<<"Tuning request issued. Handle: "<<handle<<std::endl;
    }

    if (resourceList) {
        delete resourceList;
    }
}
```
The memory allocated for the resourceList needs to be freed by the caller.


### 4.2.2. retuneResources

**Description:**
Modifies the duration of an existing tune request.

**API Signature:**
```cpp
int8_t retuneResources(int64_t handle,
                       int64_t duration);
```

**Parameters:**

- `handle` (`int64_t`): Handle of the original request, returned by the call to `tuneResources`.
- `duration` (`int64_t`): New duration in milliseconds. Use `-1` for an infinite duration.

**Returns:**
`int8_t`
- `0` if the request was successfully submitted.
- `-1` otherwise.

#### 4.2.2.1. Example

The below example demonstrates the use of the retuneResources API for modifying a request's duration.
Note: Only extension of request duration is allowed.

```cpp
void sendRequest() {
    // Modify the duration of a previously issued Tune Request to 20 seconds
    // Let's say we stored the handle returned by the tuneResources API in
    // a variable called "handle". Then the retuneResources API can be simply called like:
    if(retuneResources(handle, 20000) < 0) {
        std::cerr<<"Failed to Send retune request to URM Server"<<std::endl;
    }
}
```

### 4.2.3. untuneResources

**Description:**
Withdraws a previously issued resource provisioning (or tune) request.

**API Signature:**
```cpp
int8_t untuneResources(int64_t handle);
```

**Parameters:**

- `handle` (`int64_t`): Handle of the original request, returned by the call to `tuneResources`.

**Returns:**
`int8_t`
- `0` if the request was successfully submitted.
- `-1` otherwise.

#### 4.2.3.1. Example

The below example demonstrates the use of the untuneResources API for untuning (i.e. reverting) a previously issued tune Request.

```cpp
void sendRequest() {
    // Withdraw a Previously issued tuning request
    if(untuneResources(handle) < 0) {
        std::cerr<<"Failed to Send untune request to URM Server"<<std::endl;
    }
}
```

### 4.2.4. tuneSignal

**Description:**
Tune the signal with the given ID.

**API Signature:**
```cpp
int64_t tuneSignal(uint32_t sigId,
                   uint32_t sigType,
                   int64_t duration,
                   int32_t properties,
                   const char* appName,
                   const char* scenario,
                   int32_t numArgs,
                   uint32_t* list);
```

**Parameters:**

- `sigID` (`uint32_t`): A uniqued 32-bit (unsigned) identifier for the Signal
    - The last 16 bits (17-32) are used to specify the SigID
    - The next 8 bits (9-16) are used to specify the Signal Category
- `sigType` (`uint32_t`): Type of the signal, sigType is typically used in multimedia pipelines (camera, encoder, transcoder) where the same signal ID may represent multiple mode variants. Default value is 0.
- `duration` (`int64_t`): Duration (in milliseconds) to tune the Signal for. A value of -1 denotes infinite duration.
- `properties` (`int32_t`): Properties of the Request.
    - The last 8 bits [25 - 32] store the Request Priority (HIGH / LOW)
    - The Next 8 bits [17 - 24] represent a Boolean Flag, which indicates if the Request should be
      processed in the background (in case of Display Off or Doze Mode).
- `appName` (`const char*`): Name of the Application that is issuing the Request
- `scenario` (`const char*`): Use-Case Scenario
- `numArgs` (`int32_t`): Number of Additional Arguments to be passed as part of the Request
- `list` (`uint32_t*`): List of Additional Arguments to be passed as part of the Request

**Returns:**
`int64_t`
- A Positive Unique Handle to identify the issued Request. The handle is used for freeing the Provisioned signal later.
- `-1`: If the Request could not be sent to the server.

#### 4.2.4.1. Signal Code Construction

```text
Signals are identified (similar to Resources) via an unsigned 32-bit integer.
- SigID (LSBs 0-15)
- Sig Category (next 8 bits, 16-23)

31                 24                 16                         0
+--------------------+------------------+------------------------+
| Reserved (1 byte)  | Sig Cat (1 byte) |     SigId (2 bytes)    |
+--------------------+------------------+------------------------+
```

The macro "CONSTRUCT_SIG_CODE" can be used for generating opcodes directly if the ResType and ResID are known:
```cpp
   uint32_t sigCode = CONSTRUCT_SIG_CODE(0x0d, 0x0008);
```

### 4.2.5. untuneSignal

**Description:**
Release (or free) the signal with the given handle.

**API Signature:**
```cpp
int8_t untuneSignal(int64_t handle);
```

**Parameters:**

- `handle` (`int64_t`): Request Handle, returned by the tuneSignal API call.

**Returns:**
`int8_t`
- `0`: If the Request was successfully sent to the server.
- `-1`: Otherwise

---
<div style="page-break-after: always;"></div>

### 4.2.6. relaySignal

**Description:**
Tune the signal with the given ID.

**API Signature:**
```cpp
int8_t relaySignal(uint32_t sigId,
                   uint32_t sigType,
                   int64_t duration,
                   int32_t properties,
                   const char* appName,
                   const char* scenario,
                   int32_t numArgs,
                   uint32_t* list);
```

**Parameters:**

- `sigId` (`uint32_t`): A uniqued 32-bit (unsigned) identifier for the Signal
    - The last 16 bits (17-32) are used to specify the SigID
    - The next 8 bits (9-16) are used to specify the Signal Category
- `sigType` (`uint32_t`): Type of the signal, useful for use-case based signal filtering and selection, i.e.
                          in situations where multiple variants of the same core signal (with minor changes)
                          need to exist to support different use-case scenarios. If no such filtering is needed, pass this field as 0.
- `duration` (`int64_t`): Duration (in milliseconds)
- `properties` (`int32_t`): Properties of the Request.
    - The last 8 bits [25 - 32] store the Request Priority (HIGH / LOW)
    - The Next 8 bits [17 - 24] represent a Boolean Flag, which indicates if the Request should be
      processed in the background (in case of Display Off or Doze Mode).
- `appName` (`const char*`): Name of the Application that is issuing the Request
- `scenario` (`const char*`): Name of the Scenario that is issuing the Request
- `numArgs` (`int32_t`): Number of Additional Arguments to be passed as part of the Request
- `numArgs` (`uint32_t*`): List of Additional Arguments to be passed as part of the Request

**Returns:**
`int8_t`
- `0`: If the Request was successfully sent to the server.
- `-1`: Otherwise

---
<div style="page-break-after: always;"></div>

### 4.2.7. getProp

**Description:**
Gets a property from the config file, this is used as property config file used for enabling or disabling internal features in URM, can also be used by modules or clients to enable/disable features in their software based on property configs in URM.

**API Signature:**
```cpp
int8_t getProp(const char* prop,
               char* buffer,
               size_t buffer_size,
               const char* def_value);
```

**Parameters:**

- `prop` (`const char*`): Name of the Property to be fetched.
- `buffer` (`char*`): Pointer to a buffer to hold the result, i.e. the property value corresponding to the specified name.
- `buffer_size` (`size_t`): Size of the buffer.
- `def_value` (`const char*`): Value to be written to the buffer in case a property with the specified Name is not found in the Config Store

**Returns:**
`int8_t`
- `0` If the Property was found in the store, and successfully fetched
- `-1` otherwise.

## 4.3. Configs

URM utilises YAML files for configuration. This includes the resources, signal config files. Target can provide their own config files, which are specific to their use-case through the extension interface

### 4.3.1. Initialization Configs

Initialisation configs are mentioned in InitConfig.yaml file. This config enables URM to setup the required settings at the time of initialisation before any request processing happens. This also defines logical layer for clusters, cgroup ids, mpam partitions, etc for configs and apps to use, which promotes portability across targets and segments.

```yaml
InitConfigs:
  - ClusterMap:
    - Id: 0
      Type: little
    - Id: 1
      Type: big
    - Id: 2
      Type: prime
  - CgroupsInfo:
    - Name: "init.scope"
      Create: false
      ID: 1
    - Name: "system.slice"
      Create: false
      ID: 2
    - Name: "user.slice"
      Create: false
      ID: 3
    - Name: "focused.slice/app.slice"
      Create: true
      ID: 4
```

**Fields Description for CGroup Config**
```
| Field        | Type       | Description | Default Value |
|--------------|------------|-------------|-----------------|
| `ID`         | `int32_t`(Mandatory)| A 32-bit unique identifier for the CGroup | Not Applicable |
| `Create`     | `boolean`(Optional) | Boolean flag indicating if the CGroup     |                |
|              |                     | needs to be created by the URM server     | False          |
| `IsThreaded` | `boolean`(Optional) | Indicate if cgroup is threaded            | False          |
| `Name`       | `string` (Optional) | Descriptive name for the CGroup           | `Empty String` |
```

Common initialization configs are defined in /etc/urm/common/InitConfig.yaml

### 4.3.2. Resource Configs

Tunable resources are specified via ResourcesConfig.yaml file.

Common resource configs are defined in /etc/urm/common/ResourcesConfig.yaml.


Each resource is defined with the following fields:
**Fields Description**
| Field           | Type       | Description | Default Value |
|----------------|------------|-------------|-----------------|
| `ResID`        | `Integer` (Mandatory)   | unsigned 16-bit Resource Identifier, unique within the Resource Type. | Not Applicable |
| `ResType`       | `Integer` (Mandatory)  | unsigned 8-bit integer, indicating the Type of the Resource, for example: cpu / dcvs | Not Applicable |
| `Name`          | `string` (Optional)   | Descriptive name | `Empty String` |
| `Path`          | `string` (Optional)   | Full resource path of sysfs or procfs file path (if applicable). | `Empty String` |
| `Supported`     | `boolean` (Optional)  | Indicates if the Resource is Eligible for Provisioning. | `False` |
| `HighThreshold` | `integer (int32_t)` (Mandatory)   | Upper threshold value for the resource. | Not Applicable |
| `LowThreshold`  | `integer (int32_t)` (Mandatory)   | Lower threshold value for the resource. | Not Applicable |
| `Permissions`   | `string` (Optional)   | Type of client allowed to Provision this Resource (`system` or `third_party`). | `third_party` |
| `Modes`         | `array` (Optional)    | Display modes applicable (`"display_on"`, `"display_off"`, `"doze"`). | 0 (i.e. not supported in any Mode) |
| `Policy`        | `string`(Optional)   | Concurrency policy (`"higher_is_better"`, `"lower_is_better"`, `"instant_apply"`, `"lazy_apply"`). | `lazy_apply` |
| `Unit`        | `string`(Optional)   | Translation Unit (`"MB"`, `"GB"`, `"KHz"`, `"Hz"` etc). | `NA (multiplier = 1)` |
| `ApplyType` | `string` (Optional)  | Indicates if the resource can have different values, across different cores, clusters or cgroups. | `global` |
| `TargetsEnabled`          | `array` (Optional)   | List of Targets on which this Resource should be available for tuning | `Empty List` |
| `TargetsDisabled`          | `array` (Optional)   | List of Targets on which this Resource should not be available for tuning | `Empty List` |

```yaml
ResourceConfigs:
  - ResType: "0x1"
    ResID: "0x0"
    Name: "RESTUNE_SCHED_UTIL_CLAMP_MIN"
    Path: "/proc/sys/kernel/sched_util_clamp_min"
    Supported: true
    HighThreshold: 1024
    LowThreshold: 0
    Permissions: "third_party"
    Modes: ["display_on", "doze"]
    Policy: "higher_is_better"

  - ResType: "0x1"
    ResID: "0x1"
    Name: "RESTUNE_SCHED_UTIL_CLAMP_MAX"
    Path: "/proc/sys/kernel/sched_util_clamp_max"
    Supported: true
    HighThreshold: 1024
    LowThreshold: 0
    Permissions: "third_party"
    Modes: ["display_on", "doze"]
    Policy: "lower_is_better"
```

### 4.3.3. Properties Config

PropertiesConfig.yaml file stores various properties which are used by URM modules internally. For example, to allocate sufficient amount of memory for different types, or to determine the Pulse Monitor duration. Client can also use this as a property store to store their properties which gives it flexibility to control properties depending on the target.

```yaml
PropertyConfigs:
  - Name: resource_tuner.maximum.concurrent.requests
    Value: "60"
  - Name: resource_tuner.maximum.resources.per.request
    Value: "64"
  - Name: resource_tuner.pulse.duration
    Value: "60000"
```

**Field Descriptions**
| Field          | Type       | Description | Default Value  |
|----------------|------------|-------------|----------------|
| `Name`         | `string` (Mandatory)   | Unique name of the property | Not Applicable
| `Value`        | `integer` (Mandatory)   | The value for the property. | Not Applicable

Common resource configs are defined in /etc/urm/common/PropertiesConfig.yaml.

### 4.3.4. Signal Configs
The file SignalsConfig.yaml defines the signal configs.

**Field Descriptions**
| Field           | Type       | Description | Default Value |
|----------------|------------|-------------|---------------|
| `SigId`          | `Integer` (Mandatory)   | 16 bit unsigned Signal Identifier, unique within the signal category | Not Applicable |
| `Category`          | `Integer` (Mandatory)   | 8 bit unsigned integer, indicating the Category of the Signal, for example: Generic, App Lifecycle. | Not Applicable |
| `Type`          | `Integer` (optional)   | 32 bit unsigned integer, indicating the type of signal ex. camera encode type, no.of multiple streams in cam encode, etc | Not Applicable |
| `Name`          | `string` (Optional)  | |`Empty String` |
| `Enable`          | `boolean` (Optional)   | Indicates if the Signal is Eligible for Provisioning. | `False` |
| `TargetsEnabled`          | `array` (Optional)   | List of Targets on which this Signal can be Tuned | `Empty List` |
| `TargetsDisabled`          | `array` (Optional)   | List of Targets on which this Signal cannot be Tuned | `Empty List` |
| `Permissions`          | `array` (Optional)   | List of acceptable Client Level Permissions for tuning this Signal | `third_party` |
|`Timeout`              | `integer` (Optional) | Default Signal Tuning Duration to be used in case the Client specifies a value of 0 for duration in the tuneSignal API call. | `1 (ms)` |
| `Resources` | `array` (Mandatory) | List of Resources. | Not Applicable |

<div style="page-break-after: always;"></div>

**Example**
```yaml
# camera encode multi-stream
# encode (12+ streams)
SignalConfigs:
  - SigId: "0x0004"
    Category: "0x03"
    Type: 12
    Name: URM_SIG_CAMERA_ENCODE_MULTI_STREAMS
    Enable: true
    TargetsEnabled: ["qcm6490"]
    Permissions: ["third_party", "system"]
    Resources:
      - {ResCode: "RES_CGRP_RUN_CORES", Values: [2, 0,1,2,3]}
      - {ResCode: "RES_CGRP_RUN_CORES", Values: [3, 4,5,6,7]}
      - {ResCode: "RES_CGRP_REL_CPU_WEIGHT", Values: [3, 90]}
      - {ResCode: "RES_CGRP_HIGH_MEMORY", Values: [3, 1073741824]}
      - {ResCode: "RES_CGRP_RUN_CORES", Values: [4, 0,1,2,3,4,5,6,7]}
      - {ResCode: "RES_CGRP_REL_CPU_WEIGHT", Values: [4, 150]}
      - {ResCode: "RES_CGRP_CPU_LATENCY", Values: [4, -20]}
      - {ResCode: "RES_CGRP_LOW_MEMORY", Values: [4, 519430400]}
      - {ResCode: "RES_CGRP_MIN_MEMORY", Values: [4, 119430400]}
```
Common Signal configs are defined in /etc/urm/common/SignalsConfig.yaml.

### 4.3.5. Per App Configs
The file PerApp.yaml defines the per-app configs.

**Field Descriptions**
| Field           | Type       | Description | Default Value |
|----------------|------------|-------------|---------------|
| `App`          | `String` (Mandatory)   | Name of the App, equivalent to process "comm" | Not Applicable |
| `Threads`          | `array` (Optional)   | List of app threads (identified by their "comm" value as specified in /proc/{pid}/comm) to be considered as in-focus, hence moved to the focused-cgroup when the app (with the above identifier) is launched. | `Empty List` |
| `Configurations`   | `array` (optional)   | List of Signal Configurations indicating the signals to be acquired when this app is launched. Note: The specified signal opcodes should correspond to actual configurations in the SignalsConfig.yaml file. | `Empty List` |

<div style="page-break-after: always;"></div>

**Example**
```yaml
PerAppConfigs:
  - App: "gst-launch-"
    Threads:
       - {"cam-server": "4"}
       - {"gst-launch-": "4"}

  - App: "chrome"
    Threads:
       - {"chrome": "FOCUSED_CGROUP_IDENTIFIER"}
    Configurations: ["0x00034aab"]

```

PerApp configs should be defined in /etc/urm/custom/PerApp.yaml.

## 4.4. Urm Config Parsing
Urm provides by default some base / default configs for Resources, Signals, Init and Properties. These
configs can be extended or overriden via user-specified customizations. Urm parses the configs in the following order: Common -> Target-Specific -> Custom.

Where target-specific configs refers to configs indexed by the target identifier, these configs are unique to that target.

Custom Configs are user-specified via the Extensions Interface, as described in detail later in the section: "Customizations & Extensions".

Both target-specific and custom configs are optional.

## 5. Client CLI
URM provides a minimal CLI to interact with the server. This is provided to help with development and debugging purposes.

### 5.1. Send a Tune Request
```bash
/usr/bin/urmCli --tune --duration <> --priority <> --num <> --res <>
```
Where:
- `duration`: Duration in milliseconds for the tune request
- `priority`: Priority level for the tune request (HIGH: 0 or LOW: 1)
- `num`: Number of resources
- `res`: List of resource ResCode, ResInfo (optional) and Values to be tuned as part of this request

Example:
```bash
# Single Resource in a Request
/usr/bin/urmCli --tune --duration 5000 --priority 0 --num 1 --res "65536:700"

# Multiple Resources in single Request
/usr/bin/urmCli --tune --duration 4400 --priority 1 --num 2 --res "0x00030000:700,0x00040001:155667"

# Multi-Valued Resource
/usr/bin/urmCli --tune --duration 9500 --priority 0 --num 1 --res "0x00090002:0,0,1,3,5"

# Specifying ResInfo (useful for Core and Cluster type Resources)
/usr/bin/urmCli --tune --duration 5000 --priority 0 --num 1 --res "0x00040000#0x00000100:1620438"

# Everything at once
/usr/bin/urmCli --tune --duration 6500 --priority 0 --num 2 --res "0x00030000:800;0x00040011#0x00000101:50000,100000"
```

### 5.2. Send an Untune Request
```bash
/usr/bin/urmCli --untune --handle <>
```
Where:
- `handle`: Handle of the previously issued tune request, which needs to be untuned

Example:
```bash
/usr/bin/urmCli --untune --handle 50
```

### 5.3. Send a Retune Request
```bash
/usr/bin/urmCli --retune --handle <> --duration <>
```
Where:
- `handle`: Handle of the previously issued tune request, which needs to be retuned
- `duration`: The new duration in milliseconds for the tune request

Example:
```bash
/usr/bin/urmCli --retune --handle 7 --duration 8000
```

### 5.4. Send a getProp Request

```bash
/usr/bin/urmCli --getProp --key <>
```
Where:
- `key`: The Prop name of which the corresponding value needs to be fetched

Example:
```bash
/usr/bin/urmCli --getProp --key "urm.logging.level"
```

### 5.5. Send a tuneSignal Request

```bash
/usr/bin/urmCli --signal --scode <>
```
Where:
- `key`: The Prop name of which the corresponding value needs to be fetched

Example:
```bash
/usr/bin/urmCli --signal --scode "0x00fe0ab1"
```

<div style="page-break-after: always;"></div>

# 6. Customizations & Extensions

 - Extension Interface
 - Custom Configs
 - Extension Features

## 6.1. Extensions Interface

The URM extesnion framework allows target chipsets to extend its functionality and customize it to their use-case. Extension interface essentially provides a series of hooks to the targets or other modules to add their own custom behaviour. This is achieved through a lightweight extension interface. This happens in the initialisation phase before the service is ready for requests.

Specifically the extension interface provides the following capabilities:
- Registering custom resource handlers
- Registering custom configuration files
- Extension features

**Extension APIs**

### 6.1.1. Registering Resource Callbacks

Registers a custom resource applier (URM_REGISTER_RES_APPLIER_CB) handler with the system. This allows the framework to invoke a user-defined callback when a tune request for a specific resource opcode is encountered. A function pointer to the callback is to be registered. Now, instead of the default resource handler (provided by URM), this callback function will be called when a resource provisioning request for this particular resource opcode arrives.

Registers a custom resource teardown handler (URM_REGISTER_RES_TEAR_CB) with the system. This allows the framework to invoke a user-defined callback when an untune request for a specific resource opcode is encountered. A function pointer to the callback is to be registered. Now, instead of the normal resource handler (provided by URM), this callback function will be called when a resource deprovisioning request for this particular resource opcode arrives.

**Usage Example**

```cpp
void applyCustomCpuFreqCustom(void* res) {
    // Custom logic to apply CPU frequency
    return 0;
}

URM_REGISTER_RES_APPLIER_CB(0x00010001, applyCustomCpuFreqCustom);
```

```cpp
void resetCustomCpuFreqCustom(void* res) {
    // Custom logic to clear currently applied CPU frequency
    return 0;
}

URM_REGISTER_RES_TEAR_CB(0x00010001, resetCustomCpuFreqCustom);
```


### 6.1.2. Registering Custom Configs

Custom config files either can be placed in /etc/urm/custom or Registers a custom configuration (URM_REGISTER_CONFIG) YAML file. This enables target chipset to provide their own config files, i.e. allowing them to provide their own custom resources for example.

**Usage Example**

```cpp
URM_REGISTER_CONFIG(RESOURCE_CONFIG, "/etc/custom_dir/targetResourceConfigCustom.yaml");
```
The above line of code, will tell URM to read the resource configs from the file
"/etc/custom_dir/targetResourceConfigCustom.yaml" instead of the default file. Note: the target chipset must honour the structure of the YAML files, for them to be read and registered successfully.

Custom signal config file can be specified similarly:

```cpp
URM_REGISTER_CONFIG(SIGNALS_CONFIG, "/etc/bin/targetSignalConfigCustom.yaml");
```


## 6.2. Custom Configs
URM allows to add custom configs and override configs items. It also provides additional configs.

### 6.2.1. Initialization Configs
Custom init configs can be added to init config yaml. New init sections can be defined. For example "ClusterMap" can be redefined if your target has 4 clusters. Also you can define new section like MPAMgroupsInfo, see the example below.

```yaml
InitConfigs:
  # Logical IDs should always be arranged from lower to higher cluster capacities
  - ClusterMap:
    - Id: 0
      Type: cluster0
    - Id: 1
      Type: cluster1
    - Id: 2
      Type: cluster2
    - Id: 3
      Type: cluster3

  - MPAMgroupsInfo:
    - Name: "default"
      ID: 0
      Priority: 0
    - Name: "camera-mpam-group"
      ID: 1
      Priority: 1
    - Name: "audio-mpam-group"
      ID: 2
      Priority: 2
    - Name: "video-mpam-group"
      ID: 3
      Priority: 3

  - CacheInfo:
    - Type: L2
      NumCacheBlocks: 2
      PriorityAware: 0
    - Type: L3
      NumCacheBlocks: 1
      PriorityAware: 1
```  

Fields Description for Mpam Group Config
| Field     | Type                | Description                | Default Value  |
|-----------|---------------------|----------------------------|----------------|
| `ID`      | `int32_t`(Mandatory)| A 32-bit unique identifier |                |
|           |                     | for the Mpam Group         | Not Applicable |
| `Priority`| `int32_t`(Optional) | Mpam Group Priority        | 0              |
| `Name`    | `string` (Optional) | Descriptive name for the   |                |
|           |                     | Mpam Group                 | `Empty String` |

Fields Description for Cache Info Config

| Field           | Type                | Description                  | Default Value |
|-----------------|---------------------|------------------------------|---------------|
| `Type`          | `string` (Mandatory)| Type of cache (L2 or L3)     |               |
|                 |                     | for which config is intended | Not Applicable|
| `NumCacheBlocks`| `int32_t`(Mandatory)| Number of Cache blocks for   |               |
|                 |                     | the above mentioned type,    |               |
|                 |                     | to be managed by URM         | Not Applicable|
| `PriorityAware` | `boolean`(Optional) | Boolean flag indicating if   |               |
|                 |                     | the Cache Type supports      |               |
|                 |                     | different Priority Levels    | `false`       |

Initialisation configs can be customized. Entire init config file can be overided by
- Targets can override initialization configs (in addition to common init configs, i.e. overrides specific configs) by simply pushing its own InitConfig.yaml into /etc/urm/custom/InitConfig.yaml
- register with extension interface 
  URM_REGISTER_CONFIG(INIT_CONFIG, "/bin/InitConfigCustom.yaml");

### 6.2.2. Resource Configs
Custom resources can be added to resources config yaml. New resources can be defined. For example "MY_OWN_CPU_SCHED_RESOURCE" defined in the example below in sched resource type (you can also choose custom resource type), and ID is taken next resource ID available after upstream resources, resource ID can also be any custom ID that you choose if you want. 

```yaml
ResourceConfigs:
  - ResType: "0x3"
    ResID: "0x10"
    Name: "MY_OWN_CPU_SCHED_RESOURCE"
    Path: "/proc/sys/kernel/sched_up_down_migrate"
    Supported: true
    HighThreshold: 1024
    LowThreshold: 0
    Permissions: "third_party"
    Modes: ["display_on", "doze"]
    Policy: "higher_is_better"
```
Resources config yaml file can be given in one of the below ways
- Targets can override resource configs by simply pushing its own ResourcesConfig.yaml into /etc/urm/custom/ResourcesConfig.yaml
- Register your resources config with URM_REGISTER_CONFIG(RESOURCE_CONFIG, "/bin/targetResourceConfigCustom.yaml");

<div style="page-break-after: always;"></div>

### 6.2.3. Properties Config
Custom properties can be added to properties config yaml. Default property can be overridden or new properties can be defined. For example "resource_tuner.maximum.concurrent.requests" default property overridden with new value, and a new property "user_own_property" defined in the example below

```yaml
PropertyConfigs:
  - Name: resource_tuner.maximum.concurrent.requests
    Value: "5"
  - Name: user_own_property
    Value: "64"
```
Properties config yaml file can be given in one of the below ways
- Targets can override properties configs by placing "PropertiesConfig.yaml" into /etc/urm/custom/PropertiesConfig.yaml
- Register your property config with URM_REGISTER_CONFIG(PROPERTIES_CONFIG, "/bin/targetPropertiesConfigCustom.yaml");

<div style="page-break-after: always;"></div>

### 6.2.4. Signal Configs
Custom signal configs can be added to signals config yaml. New signal configs can be defined. For example "URM_SIG_VIDEO_DECODE" is defined in the example below, you can use custom signal category, Id, and resources. 

```yaml
SignalConfigs:
  # Video decode
  # Default decode 30 fps
  - SigId: "0x0001"
    Category: "0x03"
    Name: "URM_SIG_VIDEO_DECODE"
    Enable: true
    TargetsEnabled: ["qcm6490"]
    Permissions: ["third_party", "system"]
    Resources:
      - {ResCode: "RES_CGRP_RUN_CORES", Values: [2, 0,1,2,3]}
      - {ResCode: "RES_CGRP_RUN_CORES", Values: [3, 4,5,6]}
      - {ResCode: "RES_CGRP_REL_CPU_WEIGHT", Values: [3, 90]}
      - {ResCode: "RES_CGRP_HIGH_MEMORY", Values: [3, 1073741824]}
      - {ResCode: "RES_CGRP_RUN_CORES", Values: [4, 0,1,2,3,4,5,6]}
      - {ResCode: "RES_CGRP_REL_CPU_WEIGHT", Values: [4, 150]}
      - {ResCode: "RES_CGRP_CPU_LATENCY", Values: [4, -20]}
      - {ResCode: "RES_CGRP_LOW_MEMORY", Values: [4, 519430400]}
      - {ResCode: "RES_CGRP_MIN_MEMORY", Values: [4, 119430400]}
      - {ResCode: "RES_SCALING_MAX_FREQ", ResInfo: "CLUSTER_LITTLE_ALL_CORES", Values: [940800]}
      - {ResCode: "RES_SCALING_MAX_FREQ", ResInfo: "CLUSTER_BIG_ALL_CORES", Values: [940800]}
      - {ResCode: "RES_SCALING_MAX_FREQ", ResInfo: "CLUSTER_PLUS_ALL_CORES", Values: [940800]}
```
Signal config yaml file can be given in one of the below ways
- Targets can override signal configs by simply pushing its own SignalsConfig.yaml into /etc/urm/custom/SignalsConfig.yaml
- Register your signals config with URM_REGISTER_CONFIG(SIGNALS_CONFIG, "/bin/targetSignalConfigCustom.yaml");

<div style="page-break-after: always;"></div>

### 6.2.5. Target Configs
The file TargetConfig.yaml defines the target configs, note this is an optional config, i.e. this
file need not necessarily be provided. URM can dynamically fetch system info, like target name,
logical to physical core / cluster mapping, number of cores etc. Use this file, if you want to
provide this information explicitly. If the TargetConfig.yaml is provided, URM will always
overide default dynamically generated target information with given config and use it. Also note, there are no field-level default values available if the TargetConfig.yaml is provided. Hence if you wish to provide this file, then you'll need to provide all the complete required information.


| Field           | Type       | Description                                   | Default Value  |
|-----------------|------------|-----------------------------------------------|----------------|
| `TargetName`    | `string`   | Target Identifier                             | Not Applicable |
| `ClusterInfo`   | `array`    | Cluster ID to Type Mapping                    | Not Applicable |
| `ClusterSpread` | `array`    |  Cluster ID to Per Cluster Core Count Mapping | Not Applicable |


```yaml
TargetConfig:
  - TargetName: ["QCS9100"]
    ClusterInfo:
      - LgcId: 0
        PhyId: 4
      - LgcId: 1
        PhyId: 0
      - LgcId: 2
        PhyId: 9
      - LgcId: 3
        PhyId: 7
    ClusterSpread:
      - PhyId: 0
        NumCores: 4
      - PhyId: 4
        NumCores: 3
      - PhyId: 7
        NumCores: 2
      - PhyId: 9
        NumCores: 1
```

## 6.3. Extension Features

The file ExtFeaturesConfig.yaml defines the Extension Features, note this is an optional config, i.e. this
file need not necessarily be provided. Use this file to specify your own custom features. Each feature is associated with it's own library and an associated list of signals. Whenever a relaySignal API request is received for any of these signals, URM will forward the request to the corresponding library.
The library is required to implement the following 3 functions:
- initFeature
- tearFeature
- relayFeature
Refer the Examples section for more details on how to use the relaySignal API.

**Field Descriptions**

| Field     | Type      | Description                                 | Default Value  |
|-----------|-----------|---------------------------------------------|----------------|
| `FeatId`  | `Integer` | unsigned 32-bit Unique Feature Identifier   | Not Applicable |
| `LibPath` | `string`  | Path to the associated library              | Not Applicable |
| `Signals` | `array`   | List of signals to subscribe the feature to | Not Applicable |


**Example Config:**

```yaml
FeatureConfigs:
  - FeatId: "0x00000001"
    Name: "FEAT-1"
    LibPath: "rlib2.so"
    Description: "Simple Algorithmic Feature, defined by the BU"
    Signals: ["0x00050000", "0x00050001"]

  - FeatId: "0x00000002"
    Name: "FEAT-2"
    LibPath: "rlib1.so"
    Description: "Simple Observer-Observable Feature, defined by the BU"
    Subscribers: ["0x00050000", "0x00050002"]
```


# 7. URM Implementation Details

## 7.1. Project Structure

```text
.
├── client
├── configs
├── contextual-classifier
├── debian
├── docs
├── extensions
├── modula
├── public_headers
└── resource-tuner
```


## 7.2. Userspace Resource Manager Key Points

Userspace resource manager (URM) contains
- Userspace resource manager (URM) exposes a variery of APIs for resource tuning and use-case/scenario tuning
- These APIs can be used by apps, features and other modules
- Using these APIs, client can tune any system resource parameters like cpu, dcvs, min / max frequencies etc.
- Userspace resource manager (URM) provides

### 7.2.1. Contextual Classifier

The Contextual Classifier is an optional module designed to identify the static context of workloads (e.g., whether an application is a game, multimedia app, or browser) based on an offline-trained model.

**Key functionalities include:**
- **Process Event Monitoring:** Monitors process creation and termination events via Netlink.
- **Process Classification:** Classifies workloads (e.g., game, multimedia) using fastText (if enabled at build time). If fastText is not built, a default inference mechanism that always classifies the workload as an application.
- **Signal Generation:** Retrieves specific signal details based on classified workload types.
- **Cgroup Management:** Dynamically manages cgroups by moving application threads to designated cgroups.
- **Action Application:** Calls `ApplyActions` to send tuning requests and `RemoveActions` to untune for process events.
- **Configurability:** Influenced by configuration files such as `fasttext_model_supervised.bin`, `classifier-blocklist.txt`, and `ignore-tokens.txt`.

**Flow of Events:**

```
+---------------------------+
| Process Event Listener    |
|       (NetLinkComm)       |
+-------------+-------------+
              |
              | Catches process events (e.g., fork, exec, exit)
              v
+---------------------------+
|      HandleProcEv()       |
| (Filters and queues events)|
+-------------+-------------+
              |
              | Notifies worker thread
              v
+---------------------------+
|     ClassifierMain()      |
|    (Worker Thread)        |
+-------------+-------------+
              |
              +----(Event Type)-----+
              |                     |
              v                     v
        +------------+        +-------------+
        | CC_APP_OPEN|        | CC_APP_CLOSE|
        +-----+------+        +-----+-------+
              |                     |
              v                     v
   +-----------------------+    +---------------------------+
   | Is Ignored Process?   |--->| Move Process to Original  |
   | (classifier-blocklist)|    |   Cgroup                  |
   +----------+------------+    +---+-----------------------+
              |  No                 |
              v                     v
   +---------------------+   +---------------------------+
   |  ClassifyProcess()  |   |   RemoveActions           |
   | (MLInference model) |   |    (untuneSignal)         |
   +----------+----------+   +---------------------------+
              |
              v
   +---------------------+
   | GetSignalDetailsFor |
   |      Workload()     |
   +----------+----------+
              |
              v
   +----------------------------+
   | MoveAppThreadsToCGroup()   |
   | (Assign to Focused Cgroup) |
   +----------+-----------------+
              |
              v
   +---------------------+
   |    ApplyActions     |
   |    (tuneSignal)     |
   +---------------------+
```

### 7.2.2. Resource Tuner

- Queued requests processed by resource Tuner
- Set of Yaml config files provides extensive configuration capability
- Tuning resources provides control over system resources like CPU, Caches, GPU, etc for. Example changing the operating point of CPU DCVS min-freq to 1GHz to improve performance or limiting its max frequency to 1.5GHz to save power
- Tuning Signals dynamically provisions the system resources for a use case or scenario such as apps launches, installations, etc. in response to received signal. Resources can be configured in yaml for signals.
- Signals pick resources related to signal from SignalsConfig.yaml
- Extension interface provides a way to customize URM behaviour, by specifying custom resources, custom signals and features.
- URM uses YAML based config files, for fetching information relating to resources/signals and properties.


```

              |
              | Tune/Untune Requests
              v
+---------------------------+
|           API             |
|           ---             |
|          Client           |
+-------------+-------------+
              |
          (sockets)
              | 
              v
     .----------------.                    .--------------.
    (                  )                  (                )
    (      Server      )----(periodic)--->( Pulse monitor  )
    (                  )                  (                )
     .--------+-------.                    .--------------.
              |
              +----(Init)-----------+---------------+-----------+
              |                     |               |           |
              v                     v               |           v
        +------------+        +------------------+  |   +------------------+
        | Duplicate  |        | Upstream Configs,|  |   | Custom resources |
        |  Checker   |     	  | Resources        |  |   |    & features    |
        +-----+------+        +-----+------------+  |   +------------------+
              |                                     |
              v                                     v
        +------------+                    +--------------------+
        |  Handle    |                    |   Target specific  |
        |  Generator |                    |   configs          |
        +-----+------+                    +--------------------+
              |     
              v       
        +------------+          .----------.          +---------------+
        |  Queue     |         (  Process   )         |               |
        |  Requests  |   ...   (  Requests  )-------->|  Authorizer   |
        |            |         (            )         |               |
        +------------+          .----------.          +-------+-------+
                                                              |
                                                              v
          +--------------+      +---------------+     +---------------+
          | Concurrency  |      | Action        |     |               |
          | Coordinator  |<-----+ Thresholds    |<----+ Rate Limiter  |
          |              |      |               |     |               |
          +------+-------+      +---------------+     +---------------+
                 |
                 v
          +--------------+
          |    Action    |
          +--------------+
```


Resource Tuner architecture is captured above.

### 7.2.3. URM Initialization

- During the server initialization phase, the YAML config files are read to build up the resource registry, property store etc.
- If the target chipset has registered any custom resources, signals or custom YAML files via the extension interface, then these changes are detected during this phase itself to build up a consolidated system view, before it can start serving requests.
- During the initialization phase, memory is pre-allocated for commonly used types (via MemoryPool), and worker (thread) capacity is reserved in advance via the ThreadPool, to avoid any delays during the request processing phase.
- URM will also fetch the target details, like target name, total number of cores, logical to physical cluster / core mapping in this phase.
- If the Signals module is plugged in, it will be initialized as well and the signal configs will be parsed similarly to resource configs.
- Once all the initialization is completed, the server is ready to serve requests, a new listener thread is created for handling requests.


### 7.2.4. Request Processing

- The client can use the URM client library to send their requests.
- URM supports sockets and binders for client-server communication.
- As soon as the request is received on the server end, a handle is generated and returned to the client. This handle uniquely identifies the request and can be used for subsequent retune (retuneResources) or untune (untuneResources) API calls.
- The request is submitted to the ThreadPool for async processing.
- When the request is picked up by a worker thread (from the ThreadPool), it will decode the request message and then validate the request.
- The request verifier, will run a series of checks on the request like permission checks, and on the resources part of the request, like config value bounds check.
- Once request is verified, a duplicate check is performed, to verify if the client has already submitted the same request before. This is done so as to the improve system efficiency and performace.
- Next the request is added to an queue, which is essentially PriorityQueue, which orders requests based on their priorities (for more details on Priority Levels, refer the next Section). This is done so that the request with the highest priority is always served first.
- To handle concurrent requests for the same resource, we maintain resource level linked lists of pending requests, which are ordered according to the request priority and resource policy. This ensures that the request with the higher priority will always be applied first. For two requests with the same priority, the application order will depend on resource policy. For example, in case of resource with "higher is better" policy, the request with a higher configuration value for the resource shall take effect first.
- Once a request reaches the head of the resource level linked list, it is applied, i.e. the config value specified by this request for the resource takes effect on the corresponding sysfs node.
- A timer is created and used to keep track of a request, i.e. check if it has expired. Once it is detected that the request has expired an untune request for the same handle as this request, is automatically generated and submitted, it will take care of resetting the effected resource nodes to their original values.
- Client modules can provide their own custom resource actions for any resource. The default action provided by URM is writing to the resource sysfs node.

**Here is a more detailed explanation of the key features discussed above:**

## 7.3. Client-Level Permissions

Certain resources can be tuned only by system clients and some which have no such restrictions and can be tuned even by third party clients. The client permissions are dynamically determined, the first time it makes a request. If a client with third party permissions tries to tune a resource, which allows only clients with system permissions to tune it, then the request shall be dropped.

## 7.4. Resource-Level Policies

To ensure efficient and predictable handling of concurrent requests, each system resource is governed by one of four predefined policies. Selecting the appropriate policy helps maintain system stability, optimize performance/power, and align resource behavior with application requirements.

- Instant Apply: This policy is for resources where the latest request needs to be honored. This is kept as the default policy.
- Higher is better: This policy honors the request writing the highest value to the node. One of the cases where this makes sense is for resources that describe the upper bound value. By applying the higher-valued request, the lower-valued request is implicitly honored.
- Lower is better: Works exactly opposite of the higher is better policy.
- Lazy Apply: Sometimes, you want the resources to apply requests in a first-in-first-out manner.

## 7.5. Request-Level Priorities

As part of the tuneResources API call, client is allowed to specify a desired priority level for the request. URM supports 2 priority levels:
- High
- Low

However when multiplexed with client-level permissions, effetive request level priorities would be
- System High [SH]
- System Low [SL]
- Third Party High (or Regular High) [TPH]
- Third Party Low (or Regular Low) [TPL]

Requests with a higher priority will always be prioritized, over another request with a lower priority. Note, the request priorities are related to the client permissions. A client with system permission is allowed to acquire any priority level it wants, however a client with third party permissions can only acquire either third party high (TPH) or third party low (TPL) level of priorities. If a client with third party permissions tries to acquire a System High or System Low level of priority, then the request will not be honoured.

## 7.6. Pulse Monitor: Detection of Dead Clients and Subsequent Cleanup

To improve efficiency and conserve memory, it is essential to regularly check for dead clients and free up any system resources associated with them. This includes, untuning all (if any) ongoing tune request issued by this client and freeing up the memory used to store client specific data (Example: client's list of requests (handles), health, permissions, threads associated with the client etc). URM ensures that such clients are detected and cleaned up within 90 seconds of the client terminated.

URM performs these actions by making use of two components:
- Pulse check: scans the list of the active clients, and checks if any of the client (PID) is dead. If it finds a dead client, it schedules the cleanup by adding this PID to a queue.
- Garbage collection: When the thread runs it iterates over the GC queue and performs the cleanup.

Pulse Monitor runs on a separate thread peroidically.

## 7.7. Rate Limiter: Preventing System Abuse

URM has a rate limiter module that prevents abuse of the system by limiting the number of requests a client can make within a given time frame. This helps to prevent clients from overwhelming the system with requests and ensures that the system remains responsive and efficient. Rate limiter works on a reward/punishment methodology. Whenever a client requests the system for the first time, it is assigned a "Health" of 100. A punishment is handed over if a client makes subsequent new requests in a very short time interval (called delta, say 2 ms).
A Reward results in increasing the health of a client (not above 100), while a punishment involves decreasing the health of the client. If at any point this value of Health reaches zero then any further requests from this client wil be dropped. Value of delta, punishment and rewards are target-configurable.

## 7.8. Duplicate Checking

URM's RequestManager component is responsible for detecting any duplicate requests issued by a client, and dropping them. This is done by checking against a list of all the requests issued by a client to identify a duplicate. If it is, then the request is dropped. If it is not, then the request is added and processed. Duplicate checking helps to improve system efficiency, by saving wasteful CPU time on processing duplicates.

## 7.9. Dynamic Mapper: Logical to Physical Mapping

Logical to physical core/cluster mapping helps to achieve application code portability across different chipsets on client side. Client can specify logical values for core and cluster. URM will translate these values to their physical counterparts and apply the request accordingly. Logical to physical mapping helps to create system independent layer and helps to make the same client code interchangable across different targets.

Logical mapping entries can be found in InitConfig.yaml and can be modified if required.

Logical layer values always arranged from lower to higher cluster capacities.
If no names assigned to entries in the dynamic mapping table then cluster'number' will be the name of the cluster
for ex. LgcId 4 named as "cluster4"

below table present in InitConfigs->ClusterMap section
| LgcId  |     Name   |
|--------|------------|
|   0    |   "little" |
|   1    |   "big"    |
|   2    |   "prime"  |

URM reads machine topology and prepares logical to physical table dynamically in the init phase, similar to below one
| LgcId  |  PhyId  |
|--------|---------|
|   0    |     0   |
|   1    |     1   |
|   2    |     3   |
|   3    |     2   |

## 7.10. Display-Aware Operational Modes

The system's operational modes are influenced by the state of the device's display. To conserve power, certain system resources are optimized only when the display is active. However, for critical components that require consistent performance—such as during background processing or time-sensitive tasks, resource tuning can still be applied even when the display is off, including during low-power states like doze mode. This ensures that essential operations maintain responsiveness without compromising overall energy efficiency.

## 7.11. Crash Recovery

In case of server crash, URM ensures that all the resource sysfs nodes are restored to a sane state, i.e. they are reset to their original values. This is done by maintaining a backup of all the resource's original values, before any modification was made on behalf of the clients by urm. In the event of server crash, reset to their original values in the backup.

## 7.12. Flexible Packaging

The Users are free to pick and choose the URM modules they want for their use-case and which fit their constraints. The Framework Module is the core/central module, however if the users choose they can add on top of it other Modules: signals and profiles.

## 7.13. Pre-Allocate Capacity for efficiency

URM provides a MemoryPool component, which allows for pre-allocation of memory for certain commonly used type at the time of server initialization. This is done to improve the efficiency of the system, by reducing the number of memory allocations and deallocations that are required during the processing of requests. The allocated memory is managed as a series of blocks which can be recycled without any system call overhead. This reduces the overhead of memory allocation and deallocation, and improves the performance of the system.

Further, a ThreadPool component is provided to pre-allocate processing capacity. This is done to improve the efficiency of the system, by reducing the number of thread creation and destruction required during the processing of Requests, further ThreadPool allows for the threads to be repeatedly reused for processing different tasks.

# 8. Contact

For questions, suggestions, or contributions, feel free to reach out:

- **Email**: Maintainers.userspace-resource-moderator@qti.qualcomm.com

# 9. License

This project is licensed under the BSD 3-Clause Clear License.

<div style="page-break-after: always;"></div>
