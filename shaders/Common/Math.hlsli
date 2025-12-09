// Common Math Utilities
#ifndef MATH_HLSLI
#define MATH_HLSLI

static const float PI = 3.14159265359;
static const float TWO_PI = 6.28318530718;
static const float HALF_PI = 1.57079632679;
static const float INV_PI = 0.31830988618;
static const float EPSILON = 0.0001;

// Utility functions
float Square(float x)
{
    return x * x;
}

float3 Square(float3 x)
{
    return x * x;
}

float Pow5(float x)
{
    float x2 = x * x;
    return x2 * x2 * x;
}

// Saturate with epsilon to avoid division by zero
float SafeSaturate(float x)
{
    return clamp(x, EPSILON, 1.0);
}

#endif // MATH_HLSLI
