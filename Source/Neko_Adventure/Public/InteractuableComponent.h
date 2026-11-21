// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractuableComponent.generated.h"


UCLASS(Abstract, Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class NEKO_ADVENTURE_API UInteractuableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Cada componente hijo implementa su lógica aquí
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void Ejecutar(AActor* Instigador);
	virtual void Ejecutar_Implementation(AActor* Instigador) {}
};
