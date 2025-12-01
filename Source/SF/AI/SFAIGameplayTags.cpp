// Fill out your copyright notice in the Description page of Project Settings.


#include "SFAIGameplayTags.h"

// Fill out your copyright notice in the Description page of Project Settings.

#include "SFAIGameplayTags.h"

namespace SFGameplayTags
{
 
    // Enemy AI State Tags
    // Normal Enemy States
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_AI_State_Normal_Base, "Enemy.AI.State.Normal.Base", "Normal Enemy Base State");
    
    // Elite Enemy States
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_AI_State_Elite_Base, "Enemy.AI.State.Elite.Base", "Elite Enemy Base State");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_AI_State_Elite_Enraged, "Enemy.AI.State.Elite.Enraged", "Elite Enemy Enraged State");
    
    // Boss Enemy States
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_AI_State_Boss_Phase1, "Enemy.AI.State.Boss.Phase1", "Boss Phase 1 State");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_AI_State_Boss_Phase2, "Enemy.AI.State.Boss.Phase2", "Boss Phase 2 State");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_AI_State_Boss_Phase3, "Enemy.AI.State.Boss.Phase3", "Boss Phase 3 State");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_AI_State_Boss_Groggy, "Enemy.AI.State.Boss.Groggy", "Boss Groggy State");
    

    // Enemy AI Behaviour Tags
    // Normal Enemy Behaviours
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_AI_Behaviour_Normal_Default, "Enemy.AI.Behaviour.Normal.Default", "Normal Enemy Default Behaviour");
    
    // Elite Enemy Behaviours
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_AI_Behaviour_Elite_Default, "Enemy.AI.Behaviour.Elite.Default", "Elite Enemy Default Behaviour");
    
    
    // Boss Enemy Behaviours
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_AI_Behaviour_Boss_Phase1, "Enemy.AI.Behaviour.Boss.Phase1", "Boss Phase 1 Behaviour");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_AI_Behaviour_Boss_Phase2, "Enemy.AI.Behaviour.Boss.Phase2", "Boss Phase 2 Behaviour");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_AI_Behaviour_Boss_Phase3, "Enemy.AI.Behaviour.Boss.Phase3", "Boss Phase 3 Behaviour");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_AI_Behaviour_Boss_Groggy, "Enemy.AI.Behaviour.Boss.Groggy", "Boss Groggy Behaviour");
    
    //AI 상태 판단 태그 정의
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(AI_State_Combat, "AI.State.Combat", "AI is in combat state");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(AI_Range_Melee, "AI.Range.Melee", "Target is within melee range");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(AI_Range_Guard, "AI.Range.Guard", "Target is within guard range");
}