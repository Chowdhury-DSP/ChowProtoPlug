#include <iostream>
#include <span>

#define DLL_EXPORT extern "C"

DLL_EXPORT void test_function()
{
    std::cout << "Hello, World!" << std::endl;
}

struct Processor
{
    float z = 0.0f;
};

DLL_EXPORT void* create_processor()
{
    std::cout << "Creating processor..." << std::endl;
    return new Processor();
}

Processor* cast (void* processor)
{
    return static_cast<Processor*> (processor);
}

DLL_EXPORT void delete_processor (void* processor)
{
    std::cout << "Deleting processor..." << std::endl;
    delete cast (processor);
}

DLL_EXPORT void prepare (void* processor, double sample_rate, int samples_per_block)
{
    std::cout << "Preparing processor with sample rate: " << sample_rate << ", and buffer size: " << samples_per_block << std::endl;
}

DLL_EXPORT void reset (void* processor) noexcept
{
    cast (processor)->z = 0.0f;
}

DLL_EXPORT void process (void* processor, std::span<float> data)
{
    auto& proc = *cast (processor);

    for (auto& x : data)
    {
        const auto new_z = x;

        static constexpr auto alpha = 0.8f;
        x = alpha * x + (1.0f - alpha) * proc.z;

        proc.z = new_z;
    }

    for (auto& x : data)
        x *= 1.0f;
}
