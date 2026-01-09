#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "SFInteractionInfo.h"
#include "SFInteractionQuery.h"
#include "UObject/Interface.h"
#include "SFInteractable.generated.h"

UENUM(BlueprintType)
enum class ESFOutlineStencil : uint8
{
	None = 0 UMETA(Hidden),
    
	Blue = 250 UMETA(DisplayName = "Blue (Default)"),
	Magenta = 251 UMETA(DisplayName = "Magenta (Portal)"),
	Red = 252 UMETA(DisplayName = "Red (Danger)"),
	Cyan = 253 UMETA(DisplayName = "Cyan (Chest)"),
	Green = 254 UMETA(DisplayName = "Green (Revive)"),
	Yellow = 255 UMETA(DisplayName = "Yellow (Pickup)"),

	// 용도별 별칭
	Default = Blue UMETA(Hidden),
	Pickup = Yellow UMETA(Hidden),
	Chest = Cyan UMETA(Hidden),
	Portal = Magenta UMETA(Hidden),
	Revive = Green UMETA(Hidden),
	Enemy = Red UMETA(Hidden),
};

FORCEINLINE int32 GetStencilValue(ESFOutlineStencil Stencil)
{
	return static_cast<int32>(Stencil);
}

/**
 * 상호작용 정보를 수집하고 구성하는 헬퍼 클래스
 * 상호작용 가능한 객체에서 여러 개의 상호작용 정보를 생성할 때 사용
 * 각 정보에 자동으로 상호작용 객체 참조를 설정
 */
class FSFInteractionInfoBuilder
{
public:
	FSFInteractionInfoBuilder(TScriptInterface<ISFInteractable> InInteractable, TArray<FSFInteractionInfo>& InInteractionInfos)
		: Interactable(InInteractable)
		, InteractionInfos(InInteractionInfos) { }

public:
	void AddInteractionInfo(const FSFInteractionInfo& InteractionInfo)
	{
		FSFInteractionInfo& Entry = InteractionInfos.Add_GetRef(InteractionInfo);
		Entry.Interactable = Interactable; // 자동으로 상호작용 객체 참조 설정
	}
	
private:
	// 상호작용 가능한 객체 참조 
	TScriptInterface<ISFInteractable> Interactable;

	// 상호작용 정보들을 저장할 배열 참조 
	TArray<FSFInteractionInfo>& InteractionInfos;
};

UINTERFACE(MinimalAPI, BlueprintType, meta=(CannotImplementInterfaceInBlueprint))
class USFInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class SF_API ISFInteractable
{
	GENERATED_BODY()

public:
	// 상호작용 가능한 Actor에서 상속받아서 상호작용에 따른 부여할 어빌리티 정보를 리턴
	virtual FSFInteractionInfo GetPreInteractionInfo(const FSFInteractionQuery& InteractionQuery) const { return FSFInteractionInfo(); }
	virtual ESFOutlineStencil GetOutlineStencil() const { return ESFOutlineStencil::Default; }

	/**
	 * 플레이어 스탯을 적용한 최종 상호작용 정보를 수집하는 함수
	 * GetPreInteractionInfo()에서 얻은 기본 정보에 플레이어의 GE 스탯을 적용하여
	 * 상호작용 지속시간을 조정한 후 최종 정보를 빌더에 추가
	 */
	virtual void GatherPostInteractionInfos(const FSFInteractionQuery& InteractionQuery, FSFInteractionInfoBuilder& InteractionInfoBuilder) const
	{
		FSFInteractionInfo InteractionInfo = GetPreInteractionInfo(InteractionQuery);
	
		if (UAbilitySystemComponent* AbilitySystem = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InteractionQuery.RequestingAvatar.Get()))
		{
			// TODO : GE 스탯 적용 로직 추가 
			InteractionInfo.Duration = FMath::Max<float>(0.f, InteractionInfo.Duration);
		}
	
		InteractionInfoBuilder.AddInteractionInfo(InteractionInfo);
	}

	/**
	 * 상호작용 실행 시 게임플레이 이벤트 데이터를 커스터마이징하는 가상 함수
	 * 특정 상호작용에서 추가적인 데이터가 필요한 경우 오버라이드하여 사용
	 * @param InteractionEventTag 상호작용 이벤트 태그
	 * @param InOutEventData [입출력] 커스터마이징할 게임플레이 이벤트 데이터
	 */
	virtual void CustomizeInteractionEventData(const FGameplayTag& InteractionEventTag, FGameplayEventData& InOutEventData) const { }

	/**
	* 하이라이트 효과를 위한 메시 컴포넌트들을 반환하는 가상 함수
	*/
	UFUNCTION(BlueprintCallable)
	virtual void GetMeshComponents(TArray<UMeshComponent*>& OutMeshComponents) const { }

	UFUNCTION(BlueprintCallable)
	virtual bool CanInteraction(const FSFInteractionQuery& InteractionQuery) const;

	virtual void OnInteractActiveStarted(AActor* Interactor) {}
	virtual void OnInteractActiveEnded(AActor* Interactor) {}
	virtual void OnInteractionSuccess(AActor* Interactor) {}
	virtual int32 GetActiveInteractorCount() const { return 0; }
};
