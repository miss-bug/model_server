//*****************************************************************************
// Copyright 2021 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//*****************************************************************************

#include "sequence_manager.hpp"

#include <spdlog/spdlog.h>

namespace ovms {

const uint32_t SequenceManager::getTimeout() const {
    return timeout;
}

void SequenceManager::setTimeout(uint32_t timeout) {
    this->timeout = timeout;
}

const uint32_t SequenceManager::getMaxSequenceNumber() const {
    return maxSequenceNumber;
}

void SequenceManager::setMaxSequenceNumber(uint32_t maxSequenceNumber) {
    this->maxSequenceNumber = maxSequenceNumber;
}

std::mutex& SequenceManager::getMutex() {
    return mutex;
}

bool SequenceManager::sequenceExists(const uint64_t& sequenceId) const {
    return sequences.count(sequenceId);
}

Status SequenceManager::removeTimedOutSequences(std::chrono::steady_clock::time_point currentTime) {
    for (auto it = sequences.cbegin(); it != sequences.cend();) {
        auto& sequence = it->second;
        auto timeDiff = currentTime - sequence.getLastActivityTime();
        if (std::chrono::duration_cast<std::chrono::seconds>(timeDiff).count() > timeout)
            it = sequences.erase(it);
        else
            ++it;
    }
    return StatusCode::OK;
}

Status SequenceManager::hasSequence(const uint64_t& sequenceId) {
    if (!sequenceExists(sequenceId))
        return StatusCode::SEQUENCE_MISSING;

    if (getSequence(sequenceId).isTerminated())
        return StatusCode::SEQUENCE_TERMINATED;

    return StatusCode::OK;
}

Status SequenceManager::createSequence(const uint64_t& sequenceId) {
    /* TO DO: Generate unique ID if not provided by the client
    if (sequenceId == 0) {
    } 
    */
    if (sequenceExists(sequenceId)) {
        spdlog::debug("Sequence with provided ID already exists");
        return StatusCode::SEQUENCE_ALREADY_EXISTS;
    } else {
        spdlog::debug("Adding new sequence with ID: {}", sequenceId);
        sequences.emplace(sequenceId, sequenceId);
    }
    return StatusCode::OK;
}

Status SequenceManager::terminateSequence(const uint64_t& sequenceId) {
    auto status = hasSequence(sequenceId);
    if (!status.ok())
        return status;

    getSequence(sequenceId).setTerminated();
    return StatusCode::OK;
}

Sequence& SequenceManager::getSequence(const uint64_t& sequenceId) {
    return sequences.at(sequenceId);
}

Status SequenceManager::removeSequence(const uint64_t& sequenceId) {
    if (sequences.count(sequenceId)) {
        spdlog::debug("Removing sequence with ID: {}", sequenceId);
        sequences.erase(sequenceId);
    } else {
        spdlog::debug("Sequence with provided ID does not exists");
        return StatusCode::SEQUENCE_MISSING;
    }
    return StatusCode::OK;
}

Status SequenceManager::processRequestedSpec(SequenceProcessingSpec& sequenceProcessingSpec) {
    const uint32_t& sequenceControlInput = sequenceProcessingSpec.getSequenceControlInput();
    const uint64_t& sequenceId = sequenceProcessingSpec.getSequenceId();
    Status status;

    if (sequenceControlInput == SEQUENCE_START) {
        status = createSequence(sequenceId);
    } else if (sequenceControlInput == NO_CONTROL_INPUT) {
        status = hasSequence(sequenceId);
    } else {  // sequenceControlInput == SEQUENCE_END
        status = terminateSequence(sequenceId);
    }
    return status;
}
}  // namespace ovms
