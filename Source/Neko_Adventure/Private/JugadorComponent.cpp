// JugadorComponent.cpp

#include "JugadorComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

UJugadorComponent::UJugadorComponent()
{
	// Este componente no necesita Tick: todo funciona por eventos
	PrimaryComponentTick.bCanEverTick = false;
}

void UJugadorComponent::BeginPlay()
{
	Super::BeginPlay();

	// Empieza con la vida llena
	Vida = VidaMaxima;

	if (AActor* Propietario = GetOwner())
	{
		// El daño aplicado al jugador (ApplyDamage, por ejemplo desde el enemigo) llega aquí
		Propietario->OnTakeAnyDamage.AddDynamic(this, &UJugadorComponent::ManejarDanioRecibido);
	}
}

ACharacter* UJugadorComponent::GetPersonaje() const
{
	return Cast<ACharacter>(GetOwner());
}

float UJugadorComponent::GetPorcentajeVida() const
{
	return VidaMaxima > 0 ? static_cast<float>(Vida) / static_cast<float>(VidaMaxima) : 0.f;
}

bool UJugadorComponent::IntentarAtacar()
{
	if (!EstaVivo())
	{
		return false;
	}

	const UWorld* Mundo = GetWorld();
	if (!Mundo)
	{
		return false;
	}

	// Respeta el tiempo de espera entre ataques
	const float Ahora = Mundo->GetTimeSeconds();
	if (Ahora - TiempoUltimoAtaque < TiempoEntreAtaques)
	{
		return false;
	}
	TiempoUltimoAtaque = Ahora;

	// Avisa al Blueprint (animación) y aplica el golpe
	OnAtaque.Broadcast();
	AplicarGolpe();

	return true;
}

int32 UJugadorComponent::AplicarGolpe()
{
	AActor* Propietario = GetOwner();
	UWorld* Mundo = GetWorld();
	if (!Propietario || !Mundo)
	{
		return 0;
	}

	// La esfera va desde el jugador hasta AlcanceAtaque cm hacia delante
	const FVector Inicio = Propietario->GetActorLocation();
	const FVector Fin = Inicio + Propietario->GetActorForwardVector() * AlcanceAtaque;

	// Ignora al propio jugador
	FCollisionQueryParams Parametros(SCENE_QUERY_STAT(GolpeJugador), false, Propietario);

	// Solo busca Pawns (personajes), no paredes ni suelo
	FCollisionObjectQueryParams TiposObjeto;
	TiposObjeto.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FHitResult> Impactos;
	Mundo->SweepMultiByObjectType(Impactos, Inicio, Fin, FQuat::Identity, TiposObjeto, FCollisionShape::MakeSphere(RadioGolpe), Parametros);

	const APawn* PawnPropietario = Cast<APawn>(Propietario);
	AController* Controlador = PawnPropietario ? PawnPropietario->GetController() : nullptr;

	// Un mismo actor puede salir varias veces (varios componentes): lo golpeamos solo una vez
	TSet<AActor*> Golpeados;
	for (const FHitResult& Impacto : Impactos)
	{
		AActor* Actor = Impacto.GetActor();
		if (!Actor || Actor == Propietario || Golpeados.Contains(Actor))
		{
			continue;
		}
		Golpeados.Add(Actor);

		UGameplayStatics::ApplyDamage(Actor, static_cast<float>(Danio), Controlador, Propietario, nullptr);
		UE_LOG(LogTemp, Log, TEXT("El jugador golpeó a %s con %d de daño"), *Actor->GetName(), Danio);
	}

	if (bMostrarGolpe)
	{
		const FColor Color = Golpeados.Num() > 0 ? FColor::Red : FColor::Yellow;
		DrawDebugSphere(Mundo, Inicio, RadioGolpe, 12, Color, false, 1.f);
		DrawDebugSphere(Mundo, Fin, RadioGolpe, 12, Color, false, 1.f);
		DrawDebugLine(Mundo, Inicio, Fin, Color, false, 1.f);
	}

	return Golpeados.Num();
}

void UJugadorComponent::ManejarDanioRecibido(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	RecibirDanio(FMath::RoundToInt(Damage));
}

void UJugadorComponent::RecibirDanio(int32 Cantidad)
{
	if (!EstaVivo() || Cantidad <= 0)
	{
		return;
	}

	Vida = FMath::Max(0, Vida - Cantidad);
	UE_LOG(LogTemp, Log, TEXT("El jugador recibió %d de daño. Vida restante: %d"), Cantidad, Vida);

	OnDanioRecibido.Broadcast(Cantidad, Vida);

	if (!EstaVivo())
	{
		Morir();
	}
}

void UJugadorComponent::Morir()
{
	if (bMuerto)
	{
		return;
	}
	bMuerto = true;

	UE_LOG(LogTemp, Warning, TEXT("El jugador ha muerto"));

	OnMuerte.Broadcast();

	if (ACharacter* Personaje = GetPersonaje())
	{
		// Deja de moverse y de responder a los controles (no se destruye)
		Personaje->GetCharacterMovement()->DisableMovement();
		Personaje->DisableInput(Cast<APlayerController>(Personaje->GetController()));
	}
}