//Copyright 2022, Dakota Dawe, All rights reserved

#pragma once


#if WITH_EDITOR
#define SKG_PRINT(Message, ...)\
if (GEngine)\
{\
	const FString Test = FString::Printf(Message, ##__VA_ARGS__);\
	GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, Test);\
}

#define SKG_PRINT_SPAM(Times, Message, ...)\
if (GEngine)\
{\
	for (int32 i = 0; i < Times; ++i)\
	{\
		const FString Test = FString::Printf(Message, ##__VA_ARGS__);\
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, Test);\
	}\
}
#else
#define SKG_PRINT(Message, ...)
#define SKG_PRINT_SPAM(Times, Message, ...)
#endif