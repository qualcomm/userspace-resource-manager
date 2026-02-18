// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <iostream>

#include "ErrCodes.h"
#include "TestUtils.h"
#include "SafeOps.h"
#include "URMTests.h"

#define TEST_CLASS "COMPONENT"

// Test cases for Add function
URM_TEST(Overflow1, {
    OperationStatus status;
    // demonstrating implicit conversion by compiler resulting in proper value but still considered overflow
    int64_t result = Add(std::numeric_limits<int32_t>::max(), 2, status);
    E_ASSERT((status == OVERFLOW));
    E_ASSERT((result == std::numeric_limits<int32_t>::max()));
})

URM_TEST(Underflow1, {
    OperationStatus status;
    int32_t result = Add(std::numeric_limits<int32_t>::lowest(), -1, status);
    E_ASSERT((status == UNDERFLOW));
    E_ASSERT((result == std::numeric_limits<int32_t>::lowest()));
})

URM_TEST(PositiveNoOverflow1, {
    OperationStatus status;
    int8_t result = Add(10, 20, status);
    E_ASSERT((status == SUCCESS));
    E_ASSERT((result == 30));
})

URM_TEST(NegativeNoUnderflow1, {
    OperationStatus status;
    int8_t result = Add(-10, -20, status);
    E_ASSERT((status == SUCCESS));
    E_ASSERT((result == -30));
})

URM_TEST(IncorrectType1, {
    OperationStatus status;
    // based on the return type, -2 is assigned
    uint8_t result = Add(1,-2,status);
    E_ASSERT((status == SUCCESS));
    E_ASSERT((result == 255));
})

URM_TEST(DifferentTypes, {
    OperationStatus status;
    int8_t a = 127;
    int16_t b = 123;
    int16_t result = Add(static_cast<int16_t>(a),b,status);
    E_ASSERT((status == SUCCESS));
    E_ASSERT((result == 250));
})

// Test cases for Subtract function
URM_TEST(Overflow2, {
    OperationStatus status;
    int64_t result = Subtract(std::numeric_limits<int64_t>::max(), static_cast<int64_t>(-1), status);
    E_ASSERT((status == OVERFLOW));
    E_ASSERT((result == std::numeric_limits<int64_t>::max()));
})

URM_TEST(Underflow2, {
    OperationStatus status;
    int32_t result = Subtract(std::numeric_limits<int32_t>::lowest(), 1, status);
    E_ASSERT((status == UNDERFLOW));
    E_ASSERT((result == std::numeric_limits<int32_t>::lowest()));
})

URM_TEST(PositiveNoOverflow2, {
    OperationStatus status;
    int8_t result = Subtract(20, 10, status);
    E_ASSERT((status == SUCCESS));
    E_ASSERT((result == 10));
})

URM_TEST(NegativeNoUnderflow2, {
    OperationStatus status;
    int8_t result = Subtract(-20, -10, status);
    E_ASSERT((status == SUCCESS));
    E_ASSERT((result == -10));
})

URM_TEST(Underflow3, {
     OperationStatus status;
     int64_t result = Multiply(std::numeric_limits<int64_t>::lowest(), static_cast<int64_t>(2), status);
     E_ASSERT((status == UNDERFLOW));
     E_ASSERT((result == std::numeric_limits<int64_t>::lowest()));
})

URM_TEST(PositiveNoOverflow3, {
     OperationStatus status;
     int64_t result = Multiply(10, 20, status);
     E_ASSERT((status == SUCCESS));
     E_ASSERT((result == 200));
})

URM_TEST(DoublePositiveOverflow, {
    OperationStatus status;
    double result = Multiply(std::numeric_limits<double>::max(), 2.7, status);
    E_ASSERT((status == OVERFLOW));
    E_ASSERT((result == std::numeric_limits<double>::max()));
})

URM_TEST(DoubleUnderflow, {
    OperationStatus status;
    double result = Multiply(2.0, std::numeric_limits<double>::lowest(), status);
    E_ASSERT((status == UNDERFLOW));
    E_ASSERT((result == std::numeric_limits<double>::lowest()));
})

URM_TEST(DoublePositiveNoOverflow, {
    OperationStatus status;
    double result = Multiply(10.0, 2.0, status);
    E_ASSERT((status == SUCCESS));
    E_ASSERT((result == 20.0));
})

URM_TEST(DivByZero, {
    OperationStatus status;
    double result = Divide(10.0, 0.0, status);
    E_ASSERT((status == DIVISION_BY_ZERO));
    E_ASSERT((result == 10.0));
})

URM_TEST(PositiveOverflow, {
    OperationStatus status;
    double result = Divide(std::numeric_limits<double>::max(), 0.5, status);
    E_ASSERT((status == OVERFLOW));
    E_ASSERT((result == std::numeric_limits<double>::max()));
})

URM_TEST(Underflow4, {
    OperationStatus status;
    double result = Divide(std::numeric_limits<double>::max(), -0.5, status);
    E_ASSERT((status == UNDERFLOW));
    E_ASSERT((result == std::numeric_limits<double>::lowest()));
})

URM_TEST(TestSafeDerefMacro, {
    int32_t* int_ptr = nullptr;
    int8_t exceptionHit = false;
    try {
        SafeDeref(int_ptr);
    } catch(const std::invalid_argument& e) {
        exceptionHit = true;
    }

    E_ASSERT((exceptionHit == true));
})

URM_TEST(TestSafeAssignmentMacro, {
    int32_t* int_ptr = nullptr;
    int8_t exceptionHit = false;
    try {
        SafeAssignment(int_ptr, 57);
    } catch(const std::invalid_argument& e) {
        exceptionHit = true;
    }

    E_ASSERT((exceptionHit == true));
})

URM_TEST(TestSafeStaticCastMacro, {
    int32_t* int_ptr = nullptr;
    int8_t exceptionHit = false;
    try {
        SafeStaticCast(int_ptr, void*);
    } catch(const std::invalid_argument& e) {
        exceptionHit = true;
    }

    E_ASSERT((exceptionHit == true));
})

URM_TEST(TestValidationMacro1, {
    int32_t val = -670;
    int8_t exceptionHit = false;
    try {
        VALIDATE_GT(val, 0);
    } catch(const std::invalid_argument& e) {
        exceptionHit = true;
    }

    E_ASSERT((exceptionHit == true));
})

URM_TEST(TestValidationMacro2, {
    int32_t val = 100;
    int8_t exceptionHit = false;
    try {
        VALIDATE_GE(val, 100);
    } catch(const std::invalid_argument& e) {
        exceptionHit = true;
    }

    E_ASSERT((exceptionHit == false));
})
