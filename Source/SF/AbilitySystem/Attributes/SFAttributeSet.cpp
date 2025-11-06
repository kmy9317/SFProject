#include "SFAttributeSet.h"

#include "AbilitySystem/SFAbilitySystemComponent.h"

USFAttributeSet::USFAttributeSet()
{
}

UWorld* USFAttributeSet::GetWorld() const
{
	const UObject* Outer = GetOuter();
	check(Outer);

	return Outer->GetWorld();
}

USFAbilitySystemComponent* USFAttributeSet::GetSFAbilitySystemComponent() const
{
	return Cast<USFAbilitySystemComponent>(GetOwningAbilitySystemComponent());
}

void USFAttributeSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	
}
