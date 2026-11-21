// Fill out your copyright notice in the Description page of Project Settings.

#include "Interactuables.h"
#include "Components/BoxComponent.h"

// Sets default values
AInteractuables::AInteractuables()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    // Crear el BoxComponent y configurarlo como trigger
    TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    TriggerBox->InitBoxExtent(FVector(100.f, 100.f, 100.f));
    TriggerBox->SetCollisionProfileName(TEXT("Trigger")); // Usa el perfil "Trigger"
    TriggerBox->SetGenerateOverlapEvents(true);

    RootComponent = TriggerBox;

}

// Called when the game starts or when spawned
void AInteractuables::BeginPlay()
{
	Super::BeginPlay();

	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AInteractuables::OnOverlapBegin);

	// Buscar el componente según el enum
	FName NombreComp;
	switch (TipoSeleccionado)
	{
	case ETipoInteractuable::Curacion:   NombreComp = TEXT("Curaciones");  break;
	case ETipoInteractuable::Destruible: NombreComp = TEXT("Destruibles"); break;
	case ETipoInteractuable::Moneda:     NombreComp = TEXT("Monedas");     break;
	}

	TArray<UInteractuableComponent*> Comps;
	GetComponents<UInteractuableComponent>(Comps);
	for (UInteractuableComponent* C : Comps)
	{
		if (C->GetFName() == NombreComp)
		{
			CompActivo = C;
		}
		else
		{
			// Desactivar los demás para que no hagan nada
			C->Deactivate();
			C->SetComponentTickEnabled(false);
		}
	}
}

void AInteractuables::OnOverlapBegin(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != this && CompActivo)
	{
		CompActivo->Ejecutar(OtherActor);
	}
}

// Called every frame
void AInteractuables::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

