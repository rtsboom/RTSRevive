#pragma once
#include <intrin.h>
#include <cstdint>

namespace rr
{
	void ReportFailure(const char* expr, const char* msg, const char* file, int line) noexcept;
	void ReportFailure(const char* expr, const char* msg, uint32_t err, const char* file, int line) noexcept;
}

#define RR_CHECK(expr)												\
    do																\
    {																\
        if (!(expr))												\
        {															\
            rr::ReportFailure(#expr, nullptr, __FILE__, __LINE__);  \
            __debugbreak();											\
            std::terminate();										\
        }															\
    } while (false)

#define RR_CHECK_MSG(expr, msg)									\
    do															\
    {															\
        if (!(expr))											\
        {														\
            rr::ReportFailure(#expr, msg, __FILE__, __LINE__);	\
            __debugbreak();										\
            std::terminate();									\
        }														\
    } while (false)

#define RR_CHECK_MSG_ERR(expr, msg, err)							\
	do																\
	{																\
		if (!(expr))												\
		{															\
			rr::ReportFailure(#expr, msg, err, __FILE__, __LINE__);	\
				__debugbreak();										\
				std::terminate();									\
		}															\
	} while(false)

#ifdef _DEBUG
#define RR_ASSERT(expr)	RR_CHECK(expr)
#define RR_ASSERT_MSG(expr, msg) RR_CHECK_MSG(expr, msg)
#define RR_ASSERT_MSG_ERR(expr, msg, err) RR_CHECK_MSG_ERR(expr,msg, err)
#else
#define RR_ASSERT(expr) ((void)0)
#define RR_ASSERT_MSG(expr, msg) ((void)0)
#define RR_ASSERT_MSG_ERR(expr, msg, err) ((void)0)
#endif
