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
	
    // Suscribirse al evento de overlap
    TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AInteractuables::OnOverlapBegin);
}

void AInteractuables::OnOverlapBegin(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult
)
{
    if (OtherActor && OtherActor != this)
    {
        UE_LOG(LogTemp, Warning, TEXT("Actor %s ha entrado en el trigger"), *OtherActor->GetName());
        // Aquí puedes poner tu lógica, por ejemplo mover el actor o iniciar una animación
    }
}

// Called every frame
void AInteractuables::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

