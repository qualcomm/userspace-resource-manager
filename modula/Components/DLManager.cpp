// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "DLManager.h"

DLManager::DLManager(int32_t linkerInUse) {
    this->mLinkerInUse = linkerInUse;
    this->mHead = this->mTail = nullptr;
    this->mSize = 0;
    this->mRank = 0;
}

// Insert at end
ErrCode DLManager::insertHelper(DLRootNode* node) {
    if(node == nullptr) {
        return RC_INVALID_VALUE;
    }

    DLRootNode* tail = this->mTail;

    if(tail != nullptr) {
        this->mTail = node;
        node->setNextLinkage(this->mLinkerInUse, nullptr);
        node->setPrevLinkage(this->mLinkerInUse, tail);
        tail->setNextLinkage(this->mLinkerInUse, node);
    } else {
        // Means set head and tail to newNode
        this->mHead = node;
        this->mTail = node;
        node->setNextLinkage(this->mLinkerInUse, nullptr);
        node->setPrevLinkage(this->mLinkerInUse, nullptr);
    }

    this->mSize++;
    return RC_SUCCESS;
}

// Insert with options, covers:
// - insertion at head and insertion at n nodes from the head.
// - insertion at tail and insertion at n nodes from the tail
ErrCode DLManager::insertHelper(DLRootNode* node, DLOptions option, int32_t n) {
    if(node == nullptr) {
        return RC_INVALID_VALUE;
    }

    switch(option) {
        case DLOptions::INSERT_START: {
            DLRootNode* head = this->mHead;

            if(head != nullptr) {
                this->mHead = node;
                node->setNextLinkage(this->mLinkerInUse, head);
                node->setPrevLinkage(this->mLinkerInUse, nullptr);
                head->setPrevLinkage(this->mLinkerInUse, node);
            } else {
                node->setPrevLinkage(this->mLinkerInUse, nullptr);
                node->setNextLinkage(this->mLinkerInUse, nullptr);
                this->mHead = node;
                this->mTail = node;
            }

            this->mSize++;
            return RC_SUCCESS;
        }

        case DLOptions::INSERT_END: {
            return this->insert(node);
        }

        case DLOptions::INSERT_N_NODE_START: {
            if(n < 0) return RC_BAD_ARG;
            if(n == 0) return this->insert(node, INSERT_START);
            if(n == this->getLen() + 1) return this->insert(node);
            if(n > this->getLen()) return RC_BAD_ARG;

            int32_t curPos = 1;
            DLRootNode* curNode = this->mHead;
            while(curNode != nullptr) {
                if(curPos == n) break;
                curNode = curNode->getNextPtr(this->mLinkerInUse);
                curPos++;
            }

            DLRootNode* prevNode = curNode->getPrevPtr(this->mLinkerInUse);
            // Insert b/w prevNode and curNode
            if(prevNode == nullptr) {
                // Insert at head
                return this->insert(node, INSERT_START);
            }

            // Actual Manipulation
            prevNode->setNextLinkage(this->mLinkerInUse, node);
            node->setPrevLinkage(this->mLinkerInUse, prevNode);
            curNode->setPrevLinkage(this->mLinkerInUse, node);
            node->setNextLinkage(this->mLinkerInUse, curNode);

            this->mSize++;
            return RC_SUCCESS;
        }

        default:
            return RC_BAD_ARG;
    }

    return RC_BAD_ARG;
}

// Insert according to policy
// Begin traversing beginning at the head, until the element is inserted.
// In such iteration the policy callback is called, waiting for it to return true.
// Assume 1-based indexing, then in the n'th iteration the n'th position element will be encountered.
// If the policyCB returns true at this point, then the new node will be inserted just before this element.
// i.e. the node will be inserted at the (n - 1)th position.
// Where n is the position in the DLL, where policyCB returns true (policyCB(newNode, nth_pos_node) == true).
// Once inserted, break out of the loop
// If empty list is encountered, initialize a new head and tail for it.
ErrCode DLManager::insertWithPolicyHelper(DLRootNode* node, DLPolicy policyCB) {
    if(node == nullptr) {
        return RC_INVALID_VALUE;
    }

    DLRootNode* currNode = this->mHead;
    int8_t inserted = false;

    while(currNode != nullptr) {
        DLRootNode* currNext = currNode->getNextPtr(this->mLinkerInUse);

        if(!inserted && policyCB(node, currNode)) {
            node->setNextLinkage(this->mLinkerInUse, currNode);
            node->setPrevLinkage(this->mLinkerInUse, currNode->getPrevPtr(this->mLinkerInUse));

            if(currNode->getPrevPtr(this->mLinkerInUse) == nullptr) {
                currNode->setPrevLinkage(this->mLinkerInUse, node);
                if(this->mHead == nullptr) {
                    this->mTail = nullptr;
                    this->mHead = nullptr;
                } else {
                    this->mHead = node;
                }

            } else {
                currNode->getPrevPtr(this->mLinkerInUse)->setNextLinkage(this->mLinkerInUse, node);
                currNode->setPrevLinkage(this->mLinkerInUse, node);
            }
            inserted = true;
            break;
        }
        currNode = currNext;
    }

    if(!inserted) {
        DLRootNode* tail = this->mTail;
        node->setNextLinkage(this->mLinkerInUse, nullptr);
        node->setPrevLinkage(this->mLinkerInUse, tail);

        if(tail != nullptr) {
            tail->setNextLinkage(this->mLinkerInUse, node);
        } else {
            this->mHead = node;
        }

        this->mTail = node;
    }

    this->mSize++;
    return RC_SUCCESS;
}

ErrCode DLManager::insertAscHelper(DLRootNode* node) {
    if(this->mSavedPolicies.mAscPolicy == nullptr) {
        return RC_BAD_ARG;
    }
    return this->insertWithPolicyHelper(node, this->mSavedPolicies.mAscPolicy);
}

ErrCode DLManager::insertDescHelper(DLRootNode* node) {
    if(this->mSavedPolicies.mDescPolicy == nullptr) {
        return RC_BAD_ARG;
    }
    return this->insertWithPolicyHelper(node, this->mSavedPolicies.mDescPolicy);
}

int8_t DLManager::isNodeNthHelper(int32_t n, DLRootNode* node) {
    int32_t position = 0;
    DLRootNode* currNode = this->mHead;

    while(currNode != nullptr) {
        if(position == n) {
            return (currNode == node);
        }
        currNode = currNode->getNextPtr(this->mLinkerInUse);
        position++;
    }

    return false;
}

DLRootNode* DLManager::getNth(int32_t n) {
    int32_t position = 0;
    DLRootNode* currNode = this->mHead;

    while(currNode != nullptr) {
        if(position == n) {
            return currNode;
        }
        currNode = currNode->getNextPtr(this->mLinkerInUse);
        position++;
    }
    return nullptr;
}

int8_t DLManager::matchAgainstHelper(DLManager* target, DLPolicy cmpPolicy) {
    if(target == nullptr) return false;
    if(this->getLen() != target->getLen()) return false;

    DLRootNode* srcCur = this->mHead;
    DLRootNode* targetCur = target->mHead;

    while(srcCur != nullptr && targetCur != nullptr) {
        if(cmpPolicy == nullptr) {
            // Match raw addresses
            if(srcCur != targetCur) return false;
        } else {
            if(!cmpPolicy(srcCur, targetCur)) {
                return false;
            }
        }

        srcCur = srcCur->getNextPtr(this->mLinkerInUse);
        targetCur = targetCur->getNextPtr(target->mLinkerInUse);
    }

    if(srcCur == nullptr && targetCur == nullptr) return true;
    if(srcCur == nullptr || targetCur == nullptr) return false;
    return true;
}

int32_t DLManager::getLenHelper() {
    return this->mSize;
}

ErrCode DLManager::deleteNodeHelper(DLRootNode* node) {
    if(node == nullptr) {
        return RC_INVALID_VALUE;
    }
    if(node->getPrevPtr(this->mLinkerInUse) != nullptr) {
        node->getPrevPtr(this->mLinkerInUse)->setNextLinkage(
            this->mLinkerInUse,
            node->getNextPtr(this->mLinkerInUse)
        );
    } else {
        // Node is at the head
        this->mHead = node->getNextPtr(this->mLinkerInUse);
        if(this->mHead == nullptr) {
            this->mTail = nullptr;
        }
    }

    if(node->getNextPtr(this->mLinkerInUse) != nullptr) {
        node->getNextPtr(this->mLinkerInUse)->setPrevLinkage(
            this->mLinkerInUse,
            node->getPrevPtr(this->mLinkerInUse)
        );
    } else {
        // Node is at the tail
        this->mTail = node->getPrevPtr(this->mLinkerInUse);
        if(this->mTail == nullptr) {
            this->mHead = nullptr;
        }
    }

    this->mSize--;
    return RC_SUCCESS;
}

void DLManager::destroyHelper() {
    this->mHead = this->mTail = nullptr;
    this->mSize = 0;
}
