// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


// SafeOpsTests_mtest.cpp
#include <iostream>
#include <limits>

#define MTEST_NO_MAIN
#include "../framework/mini.hpp"

#include "ErrCodes.h"
#include "TestUtils.h"
#include "SafeOps.h"
#include "TestAggregator.h"

// ---------- Tests for Add ----------

static void Overflow1_impl(mtest::TestContext& ctx) {
    OperationStatus status;
    // implicit conversion yields proper value but is still considered overflow
    int64_t result = Add(std::numeric_limits<int32_t>::max(), 2, status);
    MT_REQUIRE(ctx, status == OVERFLOW);
    MT_REQUIRE_EQ(ctx, result, (int64_t)std::numeric_limits<int32_t>::max());
}
MT_TEST(SafeOps, Overflow1, "component") { Overflow1_impl(ctx); }

static void Underflow1_impl(mtest::TestContext& ctx) {
    OperationStatus status;
    int32_t result = Add(std::numeric_limits<int32_t>::lowest(), -1, status);
    MT_REQUIRE(ctx, status == UNDERFLOW);
    MT_REQUIRE_EQ(ctx, result, std::numeric_limits<int32_t>::lowest());
}
MT_TEST(SafeOps, Underflow1, "component") { Underflow1_impl(ctx); }

static void PositiveNoOverflow1_impl(mtest::TestContext& ctx) {
    OperationStatus status;
    int8_t result = Add(10, 20, status);
    MT_REQUIRE(ctx, status == SUCCESS);
    MT_REQUIRE_EQ(ctx, result, (int8_t)30);
}
MT_TEST(SafeOps, PositiveNoOverflow1, "component") { PositiveNoOverflow1_impl(ctx); }

static void NegativeNoUnderflow1_impl(mtest::TestContext& ctx) {
    OperationStatus status;
    int8_t result = Add(-10, -20, status);
    MT_REQUIRE(ctx, status == SUCCESS);
    MT_REQUIRE_EQ(ctx, result, (int8_t)-30);
}
MT_TEST(SafeOps, NegativeNoUnderflow1, "component") { NegativeNoUnderflow1_impl(ctx); }

static void IncorrectType1_impl(mtest::TestContext& ctx) {
    OperationStatus status;
    // based on the return type, -2 is assigned (uint8_t wraps to 255)
    uint8_t result = Add(1, -2, status);
    MT_REQUIRE(ctx, status == SUCCESS);
    MT_REQUIRE_EQ(ctx, result, (uint8_t)255);
}
MT_TEST(SafeOps, IncorrectType1, "component") { IncorrectType1_impl(ctx); }

static void DifferentTypes_impl(mtest::TestContext& ctx) {
    OperationStatus status;
    int8_t a = 127;
    int16_t b = 123;
    int16_t result = Add(static_cast<int16_t>(a), b, status);
    MT_REQUIRE(ctx, status == SUCCESS);
    MT_REQUIRE_EQ(ctx, result, (int16_t)250);
}
MT_TEST(SafeOps, DifferentTypes, "component") { DifferentTypes_impl(ctx); }

// ---------- Tests for Subtract ----------

static void Overflow2_impl(mtest::TestContext& ctx) {
    OperationStatus status;
    int64_t result = Subtract(std::numeric_limits<int64_t>::max(),
                              static_cast<int64_t>(-1), status);
    MT_REQUIRE(ctx, status == OVERFLOW);
    MT_REQUIRE_EQ(ctx, result, std::numeric_limits<int64_t>::max());
}
MT_TEST(SafeOps, Overflow2, "component") { Overflow2_impl(ctx); }

static void Underflow2_impl(mtest::TestContext& ctx) {
    OperationStatus status;
    int32_t result = Subtract(std::numeric_limits<int32_t>::lowest(), 1, status);
    MT_REQUIRE(ctx, status == UNDERFLOW);
    MT_REQUIRE_EQ(ctx, result, std::numeric_limits<int32_t>::lowest());
}
MT_TEST(SafeOps, Underflow2, "component") { Underflow2_impl(ctx); }

static void PositiveNoOverflow2_impl(mtest::TestContext& ctx) {
    OperationStatus status;
    int8_t result = Subtract(20, 10, status);
    MT_REQUIRE(ctx, status == SUCCESS);
    MT_REQUIRE_EQ(ctx, result, (int8_t)10);
}
MT_TEST(SafeOps, PositiveNoOverflow2, "component") { PositiveNoOverflow2_impl(ctx); }

static void NegativeNoUnderflow2_impl(mtest::TestContext& ctx) {
    OperationStatus status;
    int8_t result = Subtract(-20, -10, status);
    MT_REQUIRE(ctx, status == SUCCESS);
    MT_REQUIRE_EQ(ctx, result, (int8_t)-10);
}
MT_TEST(SafeOps, NegativeNoUnderflow2, "component") { NegativeNoUnderflow2_impl(ctx); }

// ---------- Tests for Multiply ----------

static void Underflow3_impl(mtest::TestContext& ctx) {
    OperationStatus status;
    int64_t result = Multiply(std::numeric_limits<int64_t>::lowest(),
                              static_cast<int64_t>(2), status);
    MT_REQUIRE(ctx, status == UNDERFLOW);
    MT_REQUIRE_EQ(ctx, result, std::numeric_limits<int64_t>::lowest());
}
MT_TEST(SafeOps, Underflow3, "component") { Underflow3_impl(ctx); }

static void PositiveNoOverflow3_impl(mtest::TestContext& ctx) {
    OperationStatus status;
    int64_t result = Multiply(10, 20, status);
    MT_REQUIRE(ctx, status == SUCCESS);
    MT_REQUIRE_EQ(ctx, result, (int64_t)200);
}
MT_TEST(SafeOps, PositiveNoOverflow3, "component") { PositiveNoOverflow3_impl(ctx); }

static void DoublePositiveOverflow_impl(mtest::TestContext& ctx) {
    OperationStatus status;
    double result = Multiply(std::numeric_limits<double>::max(), 2.7, status);
    MT_REQUIRE(ctx, status == OVERFLOW);
    MT_REQUIRE_EQ(ctx, result, std::numeric_limits<double>::max());
}
MT_TEST(SafeOps, DoublePositiveOverflow, "component") { DoublePositiveOverflow_impl(ctx); }

static void DoubleUnderflow_impl(mtest::TestContext& ctx) {
    OperationStatus status;
    double result = Multiply(2.0, std::numeric_limits<double>::lowest(), status);
    MT_REQUIRE(ctx, status == UNDERFLOW);
    MT_REQUIRE_EQ(ctx, result, std::numeric_limits<double>::lowest());
}
MT_TEST(SafeOps, DoubleUnderflow, "component") { DoubleUnderflow_impl(ctx); }

static void DoublePositiveNoOverflow_impl(mtest::TestContext& ctx) {
    OperationStatus status;
    double result = Multiply(10.0, 2.0, status);
    MT_REQUIRE(ctx, status == SUCCESS);
    MT_REQUIRE_EQ(ctx, result, 20.0);
}
MT_TEST(SafeOps, DoublePositiveNoOverflow, "component") { DoublePositiveNoOverflow_impl(ctx); }

// ---------- Tests for Divide ----------

static void DivByZero_impl(mtest::TestContext& ctx) {
    OperationStatus status;
    double result = Divide(10.0, 0.0, status);
    MT_REQUIRE(ctx, status == DIVISION_BY_ZERO);
    MT_REQUIRE_EQ(ctx, result, 10.0);
}
MT_TEST(SafeOps, DivByZero, "component") { DivByZero_impl(ctx); }

static void PositiveOverflow_impl(mtest::TestContext& ctx) {
    OperationStatus status;
    double result = Divide(std::numeric_limits<double>::max(), 0.5, status);
    MT_REQUIRE(ctx, status == OVERFLOW);
    MT_REQUIRE_EQ(ctx, result, std::numeric_limits<double>::max());
}
MT_TEST(SafeOps, PositiveOverflow, "component") { PositiveOverflow_impl(ctx); }

static void Underflow4_impl(mtest::TestContext& ctx) {
    OperationStatus status;
    double result = Divide(std::numeric_limits<double>::max(), -0.5, status);
    MT_REQUIRE(ctx, status == UNDERFLOW);
    MT_REQUIRE_EQ(ctx, result, std::numeric_limits<double>::lowest());
}
MT_TEST(SafeOps, Underflow4, "component") { Underflow4_impl(ctx); }

// ---------- Tests for Safe* macros & VALIDATE_* ----------

static void TestSafeDerefMacro_impl(mtest::TestContext& ctx) {
    int32_t* int_ptr = nullptr;
    int8_t exceptionHit = false;
    try {
        SafeDeref(int_ptr);
    } catch (const std::invalid_argument& /*e*/) {
        exceptionHit = true;
    }
    MT_REQUIRE(ctx, exceptionHit == true);
}
MT_TEST(SafeOps, TestSafeDerefMacro, "component") { TestSafeDerefMacro_impl(ctx); }

static void TestSafeAssignmentMacro_impl(mtest::TestContext& ctx) {
    int32_t* int_ptr = nullptr;
    int8_t exceptionHit = false;
    try {
        SafeAssignment(int_ptr, 57);
    } catch (const std::invalid_argument& /*e*/) {
        exceptionHit = true;
    }
    MT_REQUIRE(ctx, exceptionHit == true);
}
MT_TEST(SafeOps, TestSafeAssignmentMacro, "component") { TestSafeAssignmentMacro_impl(ctx); }

static void TestSafeStaticCastMacro_impl(mtest::TestContext& ctx) {
    int32_t* int_ptr = nullptr;
    int8_t exceptionHit = false;
    try {
        SafeStaticCast(int_ptr, void*);
    } catch (const std::invalid_argument& /*e*/) {
        exceptionHit = true;
    }
    MT_REQUIRE(ctx, exceptionHit == true);
}
MT_TEST(SafeOps, TestSafeStaticCastMacro, "component") { TestSafeStaticCastMacro_impl(ctx); }

static void TestValidationMacro1_impl(mtest::TestContext& ctx) {
    int32_t val = -670;
    int8_t exceptionHit = false;
    try {
        VALIDATE_GT(val, 0);
    } catch (const std::invalid_argument& /*e*/) {
        exceptionHit = true;
    }
    MT_REQUIRE(ctx, exceptionHit == true);
}
MT_TEST(SafeOps, TestValidationMacro1, "component") { TestValidationMacro1_impl(ctx); }

static void TestValidationMacro2_impl(mtest::TestContext& ctx) {
    int32_t val = 100;
    int8_t exceptionHit = false;
    try {
        VALIDATE_GE(val, 100);
    } catch (const std::invalid_argument& /*e*/) {
        exceptionHit = true;
    }
    MT_REQUIRE(ctx, exceptionHit == false);
}
MT_TEST(SafeOps, TestValidationMacro2, "component") { TestValidationMacro2_impl(ctx); }

