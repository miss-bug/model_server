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
#include <chrono>
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../sequence_manager.hpp"
#include "../status.hpp"
#include "stateful_test_utils.hpp"

TEST(SequenceManager, CreateSequenceOK) {
    MockedSequenceManager sequenceManager(120, 24);
    ASSERT_FALSE(sequenceManager.sequenceExists(42));
    auto status = sequenceManager.mockCreateSequence(42);
    ASSERT_TRUE(status.ok());
    ASSERT_TRUE(sequenceManager.sequenceExists(42));
}

TEST(SequenceManager, CreateSequenceConflict) {
    MockedSequenceManager sequenceManager(120, 24);
    sequenceManager.mockCreateSequence(42);
    auto status = sequenceManager.mockCreateSequence(42);
    ASSERT_TRUE(status == ovms::StatusCode::SEQUENCE_ALREADY_EXISTS);
    ASSERT_TRUE(sequenceManager.sequenceExists(42));
}

TEST(SequenceManager, RemoveSequenceOK) {
    MockedSequenceManager sequenceManager(120, 24);
    sequenceManager.mockCreateSequence(42);
    auto status = sequenceManager.removeSequence(42);
    ASSERT_TRUE(status.ok());
    ASSERT_FALSE(sequenceManager.sequenceExists(42));
}

TEST(SequenceManager, RemoveSequenceNotExists) {
    MockedSequenceManager sequenceManager(120, 24);
    auto status = sequenceManager.removeSequence(42);
    ASSERT_TRUE(status == ovms::StatusCode::SEQUENCE_MISSING);
}

TEST(SequenceManager, HasSequenceOK) {
    MockedSequenceManager sequenceManager(120, 24);
    sequenceManager.mockCreateSequence(42);
    auto status = sequenceManager.mockHasSequence(42);
    ASSERT_TRUE(status.ok());
}

TEST(SequenceManager, HasSequenceNotExist) {
    MockedSequenceManager sequenceManager(120, 24);
    auto status = sequenceManager.mockHasSequence(42);
    ASSERT_TRUE(status == ovms::StatusCode::SEQUENCE_MISSING);
}

TEST(SequenceManager, HasSequenceTerminated) {
    MockedSequenceManager sequenceManager(120, 24);
    sequenceManager.mockCreateSequence(42);
    auto status = sequenceManager.mockTerminateSequence(42);
    ASSERT_TRUE(status.ok());

    status = sequenceManager.mockHasSequence(42);
    ASSERT_TRUE(status == ovms::StatusCode::SEQUENCE_TERMINATED);
}

TEST(SequenceManager, TerminateSequenceOK) {
    MockedSequenceManager sequenceManager(120, 24);
    sequenceManager.mockCreateSequence(42);
    auto status = sequenceManager.mockTerminateSequence(42);
    ASSERT_TRUE(status.ok());
}

TEST(SequenceManager, TerminateSequenceMissing) {
    MockedSequenceManager sequenceManager(120, 24);
    auto status = sequenceManager.mockTerminateSequence(42);
    ASSERT_TRUE(status == ovms::StatusCode::SEQUENCE_MISSING);
}

TEST(SequenceManager, TerminateSequenceAlreadyTerminated) {
    MockedSequenceManager sequenceManager(120, 24);
    sequenceManager.mockCreateSequence(42);
    auto status = sequenceManager.mockTerminateSequence(42);
    ASSERT_TRUE(status.ok());

    status = sequenceManager.mockTerminateSequence(42);
    ASSERT_TRUE(status == ovms::StatusCode::SEQUENCE_TERMINATED);
}

TEST(SequenceManager, ProcessSpecNoControlInput) {
    MockedSequenceManager sequenceManager(120, 24);
    ovms::SequenceProcessingSpec spec(ovms::NO_CONTROL_INPUT, 42);
    auto status = sequenceManager.processRequestedSpec(spec);
    ASSERT_TRUE(status == ovms::StatusCode::SEQUENCE_MISSING);

    sequenceManager.mockCreateSequence(42);
    status = sequenceManager.processRequestedSpec(spec);
    ASSERT_TRUE(status.ok());

    sequenceManager.mockTerminateSequence(42);

    status = sequenceManager.processRequestedSpec(spec);
    ASSERT_TRUE(status == ovms::StatusCode::SEQUENCE_TERMINATED);
}

TEST(SequenceManager, ProcessSpecSequenceStart) {
    MockedSequenceManager sequenceManager(120, 24);
    ovms::SequenceProcessingSpec spec(ovms::SEQUENCE_START, 42);
    auto status = sequenceManager.processRequestedSpec(spec);
    ASSERT_TRUE(status.ok());

    status = sequenceManager.processRequestedSpec(spec);
    ASSERT_TRUE(status == ovms::StatusCode::SEQUENCE_ALREADY_EXISTS);
}

TEST(SequenceManager, ProcessSpecSequenceEnd) {
    MockedSequenceManager sequenceManager(120, 24);
    ovms::SequenceProcessingSpec spec(ovms::SEQUENCE_END, 42);
    auto status = sequenceManager.processRequestedSpec(spec);
    ASSERT_TRUE(status == ovms::StatusCode::SEQUENCE_MISSING);

    sequenceManager.mockCreateSequence(42);
    status = sequenceManager.processRequestedSpec(spec);
    ASSERT_TRUE(status.ok());

    status = sequenceManager.processRequestedSpec(spec);
    ASSERT_TRUE(status == ovms::StatusCode::SEQUENCE_TERMINATED);
}

TEST(SequenceManager, RemoveTimedOutSequences) {
    ovms::model_memory_state_t newState;
    std::vector<size_t> shape1{1, 10};
    size_t elementsCount1 = std::accumulate(shape1.begin(), shape1.end(), 1, std::multiplies<size_t>());
    std::vector<float> state1(elementsCount1);
    std::iota(state1.begin(), state1.end(), 0);
    addState(newState, "state1", shape1, state1);

    MockedSequenceManager sequenceManager(5, 24);
    EXPECT_EQ(sequenceManager.getTimeout(), 5);
    sequenceManager.mockCreateSequence(42);
    sequenceManager.mockCreateSequence(314);

    ASSERT_TRUE(sequenceManager.sequenceExists(42));
    ASSERT_TRUE(sequenceManager.sequenceExists(314));
    std::this_thread::sleep_for(std::chrono::seconds(3));

    sequenceManager.removeTimedOutSequences(std::chrono::steady_clock::now());
    ASSERT_TRUE(sequenceManager.sequenceExists(42));
    ASSERT_TRUE(sequenceManager.sequenceExists(314));

    sequenceManager.getSequence(42).updateMemoryState(newState);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    sequenceManager.removeTimedOutSequences(std::chrono::steady_clock::now());
    ASSERT_TRUE(sequenceManager.sequenceExists(42));
    ASSERT_FALSE(sequenceManager.sequenceExists(314));
}