#pragma once

#include "Logging/LogMacros.h"

class UObject;

SF_API DECLARE_LOG_CATEGORY_EXTERN(LogSF, Log, All);

SF_API FString GetClientServerContextString(UObject* ContextObject = nullptr);