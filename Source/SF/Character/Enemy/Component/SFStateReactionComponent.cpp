// Fill out your copyright notice in the Description page of Project Settings.


#include "SFStateReactionComponent.h"

#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "AI/SFAIGameplayTags.h"
#include "Character/SFCharacterGameplayTags.h"
#include "Character/SFPawnExtensionComponent.h"
#include "Character/Enemy/SFEnemyData.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"




void USFStateReactionComponent::Initialize(UAbilitySystemComponent* InASC)
{
	if (!IsValid(InASC))
	{
		return;
	}
	ASC = InASC;
	USFPawnExtensionComponent* PawnExtensionComponent  = USFPawnExtensionComponent::FindPawnExtensionComponent(GetOwner());
	if (PawnExtensionComponent)
	{
		if (const USFEnemyData* EnemyData = PawnExtensionComponent->GetPawnData<USFEnemyData>())
		{
			MontageContainer = EnemyData->MontageContainer;
		}
	}
	if (AActor* OwnerActor = GetOwner())
	{
		if (USkeletalMeshComponent* MeshComp = OwnerActor->FindComponentByClass<USkeletalMeshComponent>())
		{
			AnimInstance = MeshComp->GetAnimInstance();
		}
	}
	MappingStateReaction();
	BindingTagsDelegate();
}

void USFStateReactionComponent::OnTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	if (!IsValid(ASC))
	{
		return;
	}
	
	if (StateReactionMap.Contains(Tag))
	{
		if (NewCount > 0)
		{
			StateReactionMap[Tag].OnStart();
		}
		else
		{
			StateReactionMap[Tag].OnEnd();
		}
	}
}

void USFStateReactionComponent::BindingTagsDelegate()
{
	ASC->RegisterGameplayTagEvent(SFGameplayTags::Character_State_Parried, EGameplayTagEventType::NewOrRemoved)
.AddUObject(this, &USFStateReactionComponent::OnTagChanged);

	ASC->RegisterGameplayTagEvent(SFGameplayTags::Character_State_Stunned, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &USFStateReactionComponent::OnTagChanged);

	ASC->RegisterGameplayTagEvent(SFGameplayTags::Character_State_Knockback, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &USFStateReactionComponent::OnTagChanged);

	ASC->RegisterGameplayTagEvent(SFGameplayTags::Character_State_Knockdown, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &USFStateReactionComponent::OnTagChanged);

	ASC->RegisterGameplayTagEvent(SFGameplayTags::Character_State_Groggy, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &USFStateReactionComponent::OnTagChanged);


	ASC->RegisterGameplayTagEvent(SFGameplayTags::Character_State_Hit, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &USFStateReactionComponent::OnTagChanged);

	ASC->RegisterGameplayTagEvent(SFGameplayTags::Character_State_Dead, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &USFStateReactionComponent::OnTagChanged);
}

void USFStateReactionComponent::MappingStateReaction()
{
	StateReactionMap.Add(
		SFGameplayTags::Character_State_Parried,
		FStateReaction{
			[this]() { StartParried(); },
			[this]() { EndParried(); }
		});

	StateReactionMap.Add(
		SFGameplayTags::Character_State_Stunned,
		FStateReaction{
			[this]() { StartStunned(); },
			[this]() { EndStunned(); }
		});

	StateReactionMap.Add(
		SFGameplayTags::Character_State_Knockback,
		FStateReaction{
			[this]() { StartKnockback(); },
			[this]() { EndKnockback(); }
		});

	StateReactionMap.Add(
		SFGameplayTags::Character_State_Knockdown,
		FStateReaction{
			[this]() { StartKnockdown(); },
			[this]() { EndKnockdown(); }
		});

	StateReactionMap.Add(
		SFGameplayTags::Character_State_Groggy,
		FStateReaction{
			[this]() { StartGroggy(); },
			[this]() { EndGroggy(); }
		});

	StateReactionMap.Add(
		SFGameplayTags::Character_State_Hit,
		FStateReaction{
			[this]() { StartHitReact(); },
			[this]() { EndHitReact(); }
		});

	StateReactionMap.Add(
		SFGameplayTags::Character_State_Dead,
		FStateReaction{
			[this]() { StartDead(); },
			[this]() { EndDead(); }
		});
}

#pragma region StateFunctions
void USFStateReactionComponent::StartParried()
{
	OnStateStart.Broadcast(SFGameplayTags::Character_State_Parried);
	// 패리당했을 때
	if (UAnimMontage* ParriedMontage = MontageContainer.GetMontage(SFGameplayTags::Character_State_Parried))
	{
		if (IsValid(AnimInstance))
		{
			AnimInstance->Montage_Stop(0.1f); 
			AnimInstance->Montage_Play(ParriedMontage, 1.f, EMontagePlayReturnType::MontageLength, 0.f, true);
		}
	}

	// - 이동 제한 (Root Motion 또는 CharacterMovement 비활성화)
	// - 무적 프레임 부여 (연속 타격 방지)
	// - 방어력 감소 또는 취약 상태 적용
	// - 파티클 이펙트 (충격파 등)
	// - 사운드 재생 (무기 튕겨나가는 소리)
}

void USFStateReactionComponent::EndParried()
{
	OnStateEnd.Broadcast(SFGameplayTags::Character_State_Parried);
	// - 이동 복구
	// - 방어력 정상화
	// - AI라면 다음 행동 결정 (후퇴, 재공격 등)

}

void USFStateReactionComponent::StartStunned()
{
	OnStateStart.Broadcast(SFGameplayTags::Character_State_Stunned);
	// 기절 상태 - 가장 심각한 무력화
	// - 기절 애니메이션 (별이 빙빙 도는 효과 등)
	// - 모든 입력/AI 행동 차단
	// - 이동 완전 정지
	// - 현재 실행 중인 어빌리티 취소
    
	// - 무기 드롭 또는 비활성화
	// - 기절 상태 UI 표시 (적에게도)
	// - 카메라 쉐이크 (플레이어가 기절한 경우)
	// - 받는 데미지 증가 (취약 상태)
	
}

void USFStateReactionComponent::EndStunned()
{
	OnStateEnd.Broadcast(SFGameplayTags::Character_State_Stunned);
	// - 타이머 클리어
	// - 이동/입력 복구
	// - 회복 애니메이션 재생
	// - 짧은 무적 시간 부여 (기절 직후 보호)
}

void USFStateReactionComponent::StartKnockback()
{
	OnStateStart.Broadcast(SFGameplayTags::Character_State_Knockback);
	// 밀려나는 상태
	// - 넉백 애니메이션
	// - 물리 기반 밀려남 적용
    
	// - 짧은 시간 동안 제어 불가
	// - 벽이나 장애물과 충돌 시 추가 데미지
	// - 파티클 이펙트 (먼지, 충격파)
	// - 카메라 이펙트 (히트스탑, 슬로우모션)
}

void USFStateReactionComponent::EndKnockback()
{
	OnStateEnd.Broadcast(SFGameplayTags::Character_State_Knockback);
	// - 제어권 회복
	// - 착지 애니메이션 (공중이었다면)
	// - 다음 상태로 전환 준비
}

void USFStateReactionComponent::StartKnockdown()
{
	OnStateStart.Broadcast(SFGameplayTags::Character_State_Knockdown);
	// 땅에 쓰러진 상태
	// - 다운 애니메이션 (앞/뒤/옆 방향별)
	// - 충돌 설정 변경 (누워있는 상태)
    
	// - 이동 완전 불가
	// - 무적 또는 높은 방어력 부여 (다운 공격 제외)
	// - 일어나기 입력 대기 (버튼 매싱 등)
	// - 카메라 각도 조정
	// - 일정 시간 후 강제 기상
}

void USFStateReactionComponent::EndKnockdown()
{
	OnStateEnd.Broadcast(SFGameplayTags::Character_State_Knockdown);
	// - 기상 애니메이션 (여러 방향)
	// - 충돌 설정 복구
	// - 기상 후 짧은 무적 시간
	// - 체력/스태미나 약간 회복 (게임 디자인에 따라)
}

void USFStateReactionComponent::StartGroggy()
{

	OnStateStart.Broadcast(SFGameplayTags::Character_State_Groggy);
	// 비틀거리는 상태 (기절 직전)
	// - 그로기 애니메이션 (휘청거림)
	// - 이동 속도 대폭 감소 (50% 등)

    
	// - 회전 속도 감소 (제대로 조준 못함)
	// - 시야 흐림 효과 (포스트 프로세스)
	// - 어빌리티 사용 제한 (약한 공격만 가능)
	// - 방어 불가
	// - 추가 피격 시 기절로 전환
	// - UI에 그로기 게이지 표시
	// - 회복 아이템 사용으로 즉시 회복 가능
}

void USFStateReactionComponent::EndGroggy()
{
	OnStateEnd.Broadcast(SFGameplayTags::Character_State_Groggy);
	// - 이동 속도 복구
	// - 시야 정상화
	// - 모든 제약 해제
	// - 짧은 회복 모션
}

void USFStateReactionComponent::StartHitReact()
{
	OnStateStart.Broadcast(SFGameplayTags::Character_State_Hit);

	if (UAnimMontage* ParriedMontage = MontageContainer.GetMontage(SFGameplayTags::Character_State_Hit))
	{
		if (IsValid(AnimInstance))
		{
			AnimInstance->Montage_Stop(0.1f); 
			AnimInstance->Montage_Play(ParriedMontage, 1.f, EMontagePlayReturnType::MontageLength, 0.f, true);
		}
	}

	// - 파티클/사운드 (피, 스파크 등)
	// - 데미지 숫자 표시
	// - 콤보 카운터 증가
	// - 연속 피격 시 상위 상태로 전환 (그로기, 기절)
}

void USFStateReactionComponent::EndHitReact()
{
	OnStateEnd.Broadcast(SFGameplayTags::Character_State_Hit);
	// - 제어권 즉시 복구
	// - AI 재활성화
	// - 다음 행동 결정
}

void USFStateReactionComponent::StartDead()
{
	OnStateStart.Broadcast(SFGameplayTags::Character_State_Dead);
	// 사망 처리 (되돌릴 수 없음)
	// - 사망 애니메이션 (여러 종류)
	// - 모든 입력/AI 완전 차단
    
	// - 충돌 비활성화

    
	// - 무기 드롭
	// - 아이템/경험치 드롭
	// - 사망 이펙트 (혈흔, 빛 등)
	// - 사운드 (사망 비명)
	// - UI 업데이트 (킬 카운트 등)
	// - 점수/보상 지급
	// - 퀘스트 진행 체크
    
	// 일정 시간 후 시체 제거

}

void USFStateReactionComponent::EndDead()
{
	OnStateEnd.Broadcast(SFGameplayTags::Character_State_Dead);
	// 사망 상태는 보통 종료되지 않음
	// 부활 시스템이 있다면 여기서 처리
	// - 체력 일부 회복
	// - 위치 리셋
	// - 충돌 재활성화
	// - 부활 이펙트
}
#pragma endregion 