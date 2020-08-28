//*****************************************************************************
// Copyright 2020 Intel Corporation
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
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../rest_parser.hpp"
#include "test_utils.hpp"

using namespace ovms;

using namespace testing;
using ::testing::ElementsAre;

using tensorflow::DataType;
using tensorflow::DataTypeSize;

const char* predictRequestColumnNamedJson = R"({
    "inputs": {
        "inputA": [
            [
                [[1.0, 2.0],
                 [3.0, 4.0],
                 [5.0, 6.0]],
                [[7.0, 8.0],
                 [9.0, 10.0],
                 [11.0, 12.0]]
            ],
            [
                [[101.0, 102.0],
                 [103.0, 104.0],
                 [105.0, 106.0]],
                [[107.0, 108.0],
                 [109.0, 110.0],
                 [111.0, 112.0]]
            ]
        ],
        "inputB": [
            [
                [1.0, 2.0, 3.0],
                [4.0, 5.0, 6.0]
            ],
            [
                [11.0, 12.0, 13.0],
                [14.0, 15.0, 16.0]
            ]
        ]
    },
    "signature_name": "serving_default"
})";

TEST(RestParserColumn, ParseValid2Inputs) {
    RestParser parser(prepareTensors({{"inputA", {2, 2, 3, 2}},
        {"inputB", {2, 2, 3}}}));
    auto status = parser.parse(predictRequestColumnNamedJson);

    ASSERT_EQ(status, StatusCode::OK);
    EXPECT_EQ(parser.getOrder(), Order::COLUMN);
    EXPECT_EQ(parser.getFormat(), Format::NAMED);
    ASSERT_EQ(parser.getProto().inputs_size(), 2);
    ASSERT_EQ(parser.getProto().inputs().count("inputA"), 1);
    ASSERT_EQ(parser.getProto().inputs().count("inputB"), 1);
    const auto& inputA = parser.getProto().inputs().at("inputA");
    const auto& inputB = parser.getProto().inputs().at("inputB");
    EXPECT_EQ(inputA.dtype(), DataType::DT_FLOAT);
    EXPECT_EQ(inputB.dtype(), DataType::DT_FLOAT);
    EXPECT_THAT(asVector(inputA.tensor_shape()), ElementsAre(2, 2, 3, 2));
    EXPECT_THAT(asVector(inputB.tensor_shape()), ElementsAre(2, 2, 3));
    ASSERT_EQ(inputA.tensor_content().size(), 2 * 2 * 3 * 2 * DataTypeSize(DataType::DT_FLOAT));
    ASSERT_EQ(inputB.tensor_content().size(), 2 * 2 * 3 * DataTypeSize(DataType::DT_FLOAT));
    EXPECT_THAT(asVector<float>(inputA.tensor_content()), ElementsAre(
                                                              1.0, 2.0,
                                                              3.0, 4.0,
                                                              5.0, 6.0,
                                                              //-------
                                                              7.0, 8.0,
                                                              9.0, 10.0,
                                                              11.0, 12.0,
                                                              //=========
                                                              101.0, 102.0,
                                                              103.0, 104.0,
                                                              105.0, 106.0,
                                                              //---------
                                                              107.0, 108.0,
                                                              109.0, 110.0,
                                                              111.0, 112.0));
    EXPECT_THAT(asVector<float>(inputB.tensor_content()), ElementsAre(
                                                              1.0, 2.0, 3.0,
                                                              4.0, 5.0, 6.0,
                                                              //============
                                                              11.0, 12, 13.0,
                                                              14.0, 15.0, 16.0));
}

TEST(RestParserColumn, ValidShape_1x1) {
    RestParser parser(prepareTensors({{"i", {1, 1}}}));

    ASSERT_EQ(parser.parse(R"({"signature_name":"","inputs":{
        "i":[[155]]
    }})"),
        StatusCode::OK);
    EXPECT_EQ(parser.getOrder(), Order::COLUMN);
    EXPECT_EQ(parser.getFormat(), Format::NAMED);
    EXPECT_THAT(asVector(parser.getProto().inputs().at("i").tensor_shape()), ElementsAre(1, 1));
    EXPECT_THAT(asVector<float>(parser.getProto().inputs().at("i").tensor_content()), ElementsAre(155));
}

TEST(RestParserColumn, ValidShape_1x2) {
    RestParser parser(prepareTensors({{"i", {1, 2}}}));

    ASSERT_EQ(parser.parse(R"({"signature_name":"","inputs":{
        "i":[[155, 56]]
    }})"),
        StatusCode::OK);
    EXPECT_EQ(parser.getOrder(), Order::COLUMN);
    EXPECT_EQ(parser.getFormat(), Format::NAMED);
    EXPECT_THAT(asVector(parser.getProto().inputs().at("i").tensor_shape()), ElementsAre(1, 2));
    EXPECT_THAT(asVector<float>(parser.getProto().inputs().at("i").tensor_content()), ElementsAre(155, 56));
}

TEST(RestParserColumn, ValidShape_2x1) {
    RestParser parser(prepareTensors({{"i", {2, 1}}}));

    ASSERT_EQ(parser.parse(R"({"signature_name":"","inputs":{
        "i":[[155],[513]]
    }})"),
        StatusCode::OK);
    EXPECT_EQ(parser.getOrder(), Order::COLUMN);
    EXPECT_EQ(parser.getFormat(), Format::NAMED);
    EXPECT_THAT(asVector(parser.getProto().inputs().at("i").tensor_shape()), ElementsAre(2, 1));
    EXPECT_THAT(asVector<float>(parser.getProto().inputs().at("i").tensor_content()), ElementsAre(155, 513));
}

TEST(RestParserColumn, ValidShape_2x2) {
    RestParser parser(prepareTensors({{"i", {2, 2}}}));

    ASSERT_EQ(parser.parse(R"({"signature_name":"","inputs":{
        "i":[[155, 9], [513, -5]]
    }})"),
        StatusCode::OK);
    EXPECT_EQ(parser.getOrder(), Order::COLUMN);
    EXPECT_EQ(parser.getFormat(), Format::NAMED);
    EXPECT_THAT(asVector(parser.getProto().inputs().at("i").tensor_shape()), ElementsAre(2, 2));
    EXPECT_THAT(asVector<float>(parser.getProto().inputs().at("i").tensor_content()), ElementsAre(155, 9, 513, -5));
}

TEST(RestParserColumn, ValidShape_2x1x3) {
    RestParser parser(prepareTensors({{"i", {2, 1, 3}}}));

    ASSERT_EQ(parser.parse(R"({"signature_name":"","inputs":{
        "i": [
            [[5,9,2]],
            [[-5,-2,-10]]
        ]
    }})"),
        StatusCode::OK);
    EXPECT_EQ(parser.getOrder(), Order::COLUMN);
    EXPECT_EQ(parser.getFormat(), Format::NAMED);
    EXPECT_THAT(asVector(parser.getProto().inputs().at("i").tensor_shape()), ElementsAre(2, 1, 3));
    EXPECT_THAT(asVector<float>(parser.getProto().inputs().at("i").tensor_content()), ElementsAre(5, 9, 2, -5, -2, -10));
}

TEST(RestParserColumn, ValidShape_2x3x1) {
    RestParser parser(prepareTensors({{"i", {2, 3, 1}}}));

    ASSERT_EQ(parser.parse(R"({"signature_name":"","inputs":{
        "i": [
            [[5], [9], [1]],
            [[-1], [-9], [25]]
        ]
    }})"),
        StatusCode::OK);
    EXPECT_EQ(parser.getOrder(), Order::COLUMN);
    EXPECT_EQ(parser.getFormat(), Format::NAMED);
    EXPECT_THAT(asVector(parser.getProto().inputs().at("i").tensor_shape()), ElementsAre(2, 3, 1));
    EXPECT_THAT(asVector<float>(parser.getProto().inputs().at("i").tensor_content()), ElementsAre(5, 9, 1, -1, -9, 25));
}

TEST(RestParserColumn, ValidShape_2x1x2x1) {
    RestParser parser(prepareTensors({{"i", {2, 1, 2, 1}}}));

    ASSERT_EQ(parser.parse(R"({"signature_name":"","inputs":{
        "i": [
            [[[5], [2]]],
            [[[6], [18]]]
        ]
    }})"),
        StatusCode::OK);
    EXPECT_EQ(parser.getOrder(), Order::COLUMN);
    EXPECT_EQ(parser.getFormat(), Format::NAMED);
    EXPECT_THAT(asVector(parser.getProto().inputs().at("i").tensor_shape()), ElementsAre(2, 1, 2, 1));
    EXPECT_THAT(asVector<float>(parser.getProto().inputs().at("i").tensor_content()), ElementsAre(5, 2, 6, 18));
}

TEST(RestParserColumn, ValidShape_2x1x3x1x5) {
    RestParser parser(prepareTensors({{"i", {2, 1, 3, 1, 5}}}));

    ASSERT_EQ(parser.parse(R"({"signature_name":"","inputs":{
        "i": [
            [[[[1, 2, 3, 4, 5]],
              [[1, 2, 3, 4, 5]],
              [[1, 2, 3, 4, 5]]]],
            [[[[1, 2, 3, 4, 5]],
              [[1, 2, 3, 4, 5]],
              [[1, 2, 3, 4, 5]]]]
        ]
    }})"),
        StatusCode::OK);
    EXPECT_EQ(parser.getOrder(), Order::COLUMN);
    EXPECT_EQ(parser.getFormat(), Format::NAMED);
    EXPECT_THAT(asVector(parser.getProto().inputs().at("i").tensor_shape()), ElementsAre(2, 1, 3, 1, 5));
    EXPECT_THAT(asVector<float>(parser.getProto().inputs().at("i").tensor_content()), ElementsAre(
                                                                                          1, 2, 3, 4, 5,
                                                                                          1, 2, 3, 4, 5,
                                                                                          1, 2, 3, 4, 5,
                                                                                          1, 2, 3, 4, 5,
                                                                                          1, 2, 3, 4, 5,
                                                                                          1, 2, 3, 4, 5));
}

TEST(RestParserColumn, AllowsDifferent0thDimension) {
    RestParser parser(prepareTensors({{"i", {2, 1, 2, 2}},
        {"j", {1, 1, 2, 2}}}));

    ASSERT_EQ(parser.parse(R"({"signature_name":"","inputs":{
        "i": [
            [[[5, 2], [10, 7]]],
            [[[5, 2], [10, 7]]]
        ],
        "j": [
            [[[5, 2], [10, 7]]]
        ]
    }})"),
        StatusCode::OK);
    EXPECT_EQ(parser.getOrder(), Order::COLUMN);
    EXPECT_EQ(parser.getFormat(), Format::NAMED);
    EXPECT_THAT(asVector(parser.getProto().inputs().at("i").tensor_shape()), ElementsAre(2, 1, 2, 2));
    EXPECT_THAT(asVector(parser.getProto().inputs().at("j").tensor_shape()), ElementsAre(1, 1, 2, 2));
    EXPECT_THAT(asVector<float>(parser.getProto().inputs().at("i").tensor_content()), ElementsAre(5, 2, 10, 7, 5, 2, 10, 7));
    EXPECT_THAT(asVector<float>(parser.getProto().inputs().at("j").tensor_content()), ElementsAre(5, 2, 10, 7));
}

TEST(RestParserColumn, ParseUint8) {
    RestParser parser(prepareTensors({{"i", {1, 1, 4}}}, InferenceEngine::Precision::U8));
    ASSERT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[[[0,5,15,255]]]}})"), StatusCode::OK);
    EXPECT_THAT(asVector<uint8_t>(parser.getProto().inputs().at("i").tensor_content()), ElementsAre(0, 5, 15, 255));
    parser = RestParser(prepareTensors({{"i", {1, 1, 4}}}, InferenceEngine::Precision::U8));
    ASSERT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[[[0.0,5.0,15.0,255.0]]]}})"), StatusCode::OK);
    EXPECT_THAT(asVector<uint8_t>(parser.getProto().inputs().at("i").tensor_content()), ElementsAre(0, 5, 15, 255));
}

TEST(RestParserColumn, ParseInt8) {
    RestParser parser(prepareTensors({{"i", {1, 1, 4}}}, InferenceEngine::Precision::I8));
    ASSERT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[[[0,-5,127,-128]]]}})"), StatusCode::OK);
    EXPECT_THAT(asVector<int8_t>(parser.getProto().inputs().at("i").tensor_content()), ElementsAre(0, -5, 127, -128));
    parser = RestParser(prepareTensors({{"i", {1, 1, 4}}}, InferenceEngine::Precision::I8));
    ASSERT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[[[0.0,-5.0,127.0,-128.0]]]}})"), StatusCode::OK);
    EXPECT_THAT(asVector<int8_t>(parser.getProto().inputs().at("i").tensor_content()), ElementsAre(0, -5, 127, -128));
}

TEST(RestParserColumn, ParseUint16) {
    RestParser parser(prepareTensors({{"i", {1, 1, 4}}}, InferenceEngine::Precision::U16));
    ASSERT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[[[0,5,128,65535]]]}})"), StatusCode::OK);
    EXPECT_THAT(asVector(parser.getProto().mutable_inputs()->at("i").mutable_int_val()), ElementsAre(0, 5, 128, 65535));
    parser = RestParser(prepareTensors({{"i", {1, 1, 4}}}, InferenceEngine::Precision::U16));
    ASSERT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[[[0.0,5.0,128.0,65535.0]]]}})"), StatusCode::OK);
    EXPECT_THAT(asVector(parser.getProto().mutable_inputs()->at("i").mutable_int_val()), ElementsAre(0, 5, 128, 65535));
}

TEST(RestParserColumn, ParseInt16) {
    RestParser parser(prepareTensors({{"i", {1, 1, 4}}}, InferenceEngine::Precision::I16));
    ASSERT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[[[0,-5,32768,-32767]]]}})"), StatusCode::OK);
    EXPECT_THAT(asVector<int16_t>(parser.getProto().inputs().at("i").tensor_content()), ElementsAre(0, -5, 32768, -32767));
    parser = RestParser(prepareTensors({{"i", {1, 1, 4}}}, InferenceEngine::Precision::I16));
    ASSERT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[[[0.0,-5.0,32768.0,-32767.0]]]}})"), StatusCode::OK);
    EXPECT_THAT(asVector<int16_t>(parser.getProto().inputs().at("i").tensor_content()), ElementsAre(0, -5, 32768, -32767));
}

TEST(RestParserColumn, ParseInt32) {
    RestParser parser(prepareTensors({{"i", {1, 1, 4}}}, InferenceEngine::Precision::I32));
    ASSERT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[[[0,-5,2147483648,-2147483647]]]}})"), StatusCode::OK);
    EXPECT_THAT(asVector<int32_t>(parser.getProto().inputs().at("i").tensor_content()), ElementsAre(0, -5, 2147483648, -2147483647));
    parser = RestParser(prepareTensors({{"i", {1, 1, 4}}}, InferenceEngine::Precision::I32));
    ASSERT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[[[0.0,-5.0,2147483648.0,-2147483647.0]]]}})"), StatusCode::OK);
    EXPECT_THAT(asVector<int32_t>(parser.getProto().inputs().at("i").tensor_content()), ElementsAre(0, -5, 2147483648, -2147483647));
}

TEST(RestParserColumn, ParseUint64) {
    RestParser parser(prepareTensors({{"i", {1, 1, 4}}}, InferenceEngine::Precision::U64));
    ASSERT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[[[0,5,128,18446744073709551615]]]}})"), StatusCode::OK);
    EXPECT_THAT(asVector<uint64_t>(parser.getProto().inputs().at("i").tensor_content()), ElementsAre(0, 5, 128, 18446744073709551615U));
    parser = RestParser(prepareTensors({{"i", {1, 1, 4}}}, InferenceEngine::Precision::U64));
    ASSERT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[[[0.0,5.0,128.0,555222.0]]]}})"), StatusCode::OK);
    EXPECT_THAT(asVector<uint64_t>(parser.getProto().inputs().at("i").tensor_content()), ElementsAre(0, 5, 128, 555222));  // Can't looselessly cast large double to int64
}

TEST(RestParserColumn, ParseInt64) {
    RestParser parser(prepareTensors({{"i", {1, 1, 4}}}, InferenceEngine::Precision::I64));
    ASSERT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[[[0,-5,5522,-9223372036854775807]]]}})"), StatusCode::OK);
    EXPECT_THAT(asVector<int64_t>(parser.getProto().inputs().at("i").tensor_content()), ElementsAre(0, -5, 5522, -9223372036854775807));
    parser = RestParser(prepareTensors({{"i", {1, 1, 4}}}, InferenceEngine::Precision::I64));
    ASSERT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[[[0.0,-5.0,5522.0,-55333.0]]]}})"), StatusCode::OK);
    EXPECT_THAT(asVector<int64_t>(parser.getProto().inputs().at("i").tensor_content()), ElementsAre(0, -5, 5522, -55333));  // Can't looselessly cast double to int64
}

TEST(RestParserColumn, ParseFloat) {
    RestParser parser(prepareTensors({{"i", {1, 1, 4}}}, InferenceEngine::Precision::FP32));
    ASSERT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[[[-5, 0, -4, 155234]]]}})"), StatusCode::OK);
    EXPECT_THAT(asVector<float>(parser.getProto().inputs().at("i").tensor_content()), ElementsAre(-5, 0, -4, 155234));
    parser = RestParser(prepareTensors({{"i", {1, 1, 4}}}, InferenceEngine::Precision::FP32));
    ASSERT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[[[-5.12, 0.4344, -4.521, 155234.221]]]}})"), StatusCode::OK);
    EXPECT_THAT(asVector<float>(parser.getProto().inputs().at("i").tensor_content()), ElementsAre(-5.12, 0.4344, -4.521, 155234.221));
}

TEST(RestParserColumn, ParseHalf) {
    RestParser parser(prepareTensors({{"i", {1, 1, 4}}}, InferenceEngine::Precision::FP16));
    ASSERT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[[[-5, 0, -4, 155234]]]}})"), StatusCode::OK);
    // EXPECT_THAT(asVector(parser.getProto().mutable_inputs()->at("i").mutable_half_val()), ElementsAre(-5, 0, -4, 155234));  // TODO: Check for values
    parser = RestParser(prepareTensors({{"i", {1, 1, 4}}}, InferenceEngine::Precision::FP16));
    ASSERT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[[[-5.1222, 0.434422, -4.52122, 155234.22122]]]}})"), StatusCode::OK);
    // EXPECT_THAT(asVector(parser.getProto().mutable_inputs()->at("i").mutable_half_val()), ElementsAre(-5.1222, 0.434422, -4.52122, 155234.22122));  // TODO: Check for values
}

TEST(RestParserColumn, InputsNotAnObject) {
    RestParser parser;

    EXPECT_EQ(parser.parse(R"({"signature_name":"","inputs":"string"})"), StatusCode::REST_INPUTS_NOT_AN_OBJECT);
    EXPECT_EQ(parser.parse(R"({"signature_name":"","inputs":5})"), StatusCode::REST_INPUTS_NOT_AN_OBJECT);
}

TEST(RestParserColumn, NoInputsFound) {
    RestParser parser;

    EXPECT_EQ(parser.parse(R"({"signature_name":"","inputs":{}})"), StatusCode::REST_NO_INPUTS_FOUND);
}

TEST(RestParserColumn, CannotParseInput) {
    RestParser parser(prepareTensors({{"i", {2, 1}}}));

    EXPECT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":2}})"), StatusCode::REST_COULD_NOT_PARSE_INPUT);
    EXPECT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":null}})"), StatusCode::REST_COULD_NOT_PARSE_INPUT);
    EXPECT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[1,null]}})"), StatusCode::REST_COULD_NOT_PARSE_INPUT);
    EXPECT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[[1,2],[3,"str"]]}})"), StatusCode::REST_COULD_NOT_PARSE_INPUT);
}

TEST(RestParserColumn, InputNotNdArray_1) {
    RestParser parser(prepareTensors({{"i", {1, 2, 3, 2}}}));

    // [1, 4, 5] size is 3 instead of 2 to be valid
    EXPECT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[
        [[[1, 2],
          [1, 3],
          [1, 4, 5]],
         [[5, 8],
          [9, 3],
          [1, 4]]]
    ]}})"),
        StatusCode::REST_COULD_NOT_PARSE_INPUT);
}

TEST(RestParserColumn, InputNotNdArray_2) {
    RestParser parser(prepareTensors({{"i", {1, 2, 3, 3}}}));

    EXPECT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[
        [[[1, 2, [8]],
          [1, 3, [3]],
          [1, 4, [5]]],
         [[5, 8, [-1]],
          [9, 3, [-5]],
          [1, 4, [-4]]]]
    ]}})"),
        StatusCode::REST_COULD_NOT_PARSE_INPUT);
}

TEST(RestParserColumn, InputNotNdArray_3) {
    RestParser parser(prepareTensors({{"i", {1, 4, 3, 2}}}));

    EXPECT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[
        [[[1, 2],
          [1, 3],
          [1, 4]],
 
         [[1, 2]],
 
         [[5, 8],
          [9, 3],
          [1, 4]],
 
         [[5, 8]]]
    ]}})"),
        StatusCode::REST_COULD_NOT_PARSE_INPUT);
}

TEST(RestParserColumn, InputNotNdArray_4) {
    RestParser parser(prepareTensors({{"i", {1, 2, 3, 2}}}));

    // [5, 6] is not a number but array
    EXPECT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[
        [[[1, 2],
          [1, 3],
          [1, 4, [5, 6]]],
         [[5, 8],
          [9, 3],
          [1, 4]]]
    ]}})"),
        StatusCode::REST_COULD_NOT_PARSE_INPUT);
}

TEST(RestParserColumn, InputNotNdArray_5) {
    RestParser parser(prepareTensors({{"i", {1, 2, 3, 2}}}));

    // [1] is of wrong shape
    EXPECT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[
        [[[1],
          [1, 2],
          [1, 3],
          [1, 4]],
         [[5, 8],
          [9, 3],
          [1, 4]]]
    ]}})"),
        StatusCode::REST_COULD_NOT_PARSE_INPUT);
}

TEST(RestParserColumn, InputNotNdArray_6) {
    RestParser parser(prepareTensors({{"i", {1, 2, 2, 2}}}));

    // [1, 1] missing - 2x2, 2x3
    EXPECT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[
        [[[1, 2],
          [1, 3]],
         [[5, 8],
          [9, 3],
          [1, 4]]]
    ]}})"),
        StatusCode::REST_COULD_NOT_PARSE_INPUT);
}

TEST(RestParserColumn, InputNotNdArray_7) {
    RestParser parser(prepareTensors({{"i", {1, 2, 3, 2}}}));

    // [1, 5] numbers are on wrong level
    EXPECT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[
        [[1, 5],
         [[1, 1],
          [1, 2],
          [1, 3]],
         [[5, 8],
          [9, 3],
          [1, 4]]]
    ]}})"),
        StatusCode::REST_COULD_NOT_PARSE_INPUT);
}

TEST(RestParserColumn, InputNotNdArray_8) {
    RestParser parser(prepareTensors({{"i", {1, 2, 3, 2}}}));

    // [1, 2], [9, 3] numbers are on wrong level
    EXPECT_EQ(parser.parse(R"({"signature_name":"","inputs":{"i":[
        [[[1, 1],
          [[1, 2]],
          [1, 3]],
         [[5, 8],
          [[9, 3]],
          [1, 4]]]
    ]}})"),
        StatusCode::REST_COULD_NOT_PARSE_INPUT);
}

TEST(RestParserColumn, InstancesShapeDiffer_1) {
    RestParser parser(prepareTensors({{"i", {2, 2, 3, 2}}}));

    // 2x3x2 vs 2x2x2
    EXPECT_EQ(parser.parse(R"({"signature_name":"","inputs":{
        "i": [
            [
                [[1, 1],
                 [1, 2],
                 [1, 3]],
                [[5, 8],
                 [9, 3],
                 [1, 4]]
            ],
            [
                [[1, 1],
                 [1, 2]],
                [[5, 8],
                 [9, 3]]
            ]
        ]
    }})"),
        StatusCode::REST_COULD_NOT_PARSE_INPUT);
}

TEST(RestParserColumn, InstancesShapeDiffer_2) {
    RestParser parser(prepareTensors({{"i", {2, 2, 3, 2}}}));

    // 2x3x2 vs 2x3x3
    EXPECT_EQ(parser.parse(R"({"signature_name":"","inputs":{
        "i": [
            [
                [[1, 1],
                 [1, 2],
                 [1, 3]],
                [[5, 8],
                 [9, 3],
                 [1, 4]]
            ],
            [
                [[1, 1, 3],
                 [1, 2, 2],
                 [1, 3, 9]],
                [[5, 8, 8],
                 [9, 3, 3],
                 [1, 4, 10]]
            ]
        ]
    }})"),
        StatusCode::REST_COULD_NOT_PARSE_INPUT);
}

TEST(RestParserColumn, InstancesShapeDiffer_3) {
    RestParser parser(prepareTensors({{"i", {2, 2, 3, 2}}}));

    // 2x3x2 vs 1x2x3x2
    EXPECT_EQ(parser.parse(R"({"signature_name":"","inputs":{
        "i": [
            [
                [[1, 1],
                 [1, 2],
                 [1, 3]],
                [[5, 8],
                 [9, 3],
                 [1, 4]]
            ],
            [[
                [[1, 1],
                 [1, 2],
                 [1, 3]],
                [[5, 8],
                 [9, 3],
                 [1, 4]]
            ]]
        ]
    }})"),
        StatusCode::REST_COULD_NOT_PARSE_INPUT);
}