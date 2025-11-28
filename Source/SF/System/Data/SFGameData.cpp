#include "SFGameData.h"

#include "System/SFAssetManager.h"

USFGameData::USFGameData()
{
	
}

const USFGameData& USFGameData::Get()
{
	return USFAssetManager::Get().GetGameData();
}

