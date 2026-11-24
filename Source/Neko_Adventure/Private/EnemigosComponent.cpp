// EnemigosComponent.cpp

#include "EnemigosComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

UEnemigosComponent::UEnemigosComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UEnemigosComponent::OnRegister()
{
	Super::OnRegister();
	ActualizarVelocidad();
}

void UEnemigosComponent::BeginPlay()
{
	Super::BeginPlay();

	// Guarda la vida con la que empieza para poder calcular el porcentaje // NUEVO
	VidaInicial = Vida; // NUEVO

	ActualizarVelocidad();

	if (AActor* Propietario = GetOwner())
	{
		// El daño aplicado al actor (ApplyDamage) llega a este componente
		Propietario->OnTakeAnyDamage.AddDynamic(this, &UEnemigosComponent::ManejarDanioRecibido);

		if (!GetPersonaje())
		{
			UE_LOG(LogTemp, Warning, TEXT("%s: el componente EnemigosComponent está en %s, que no es un Character. La velocidad no se aplicará."),
				*GetName(), *Propietario->GetName());
		}
	}
}

void UEnemigosComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bMostrarRadioDeteccion && GetOwner())
	{
		const FVector Centro = GetOwner()->GetActorLocation();

		const FColor Color = JugadorEnRango() ? FColor::Red : FColor::Green;
		DrawDebugSphere(GetWorld(), Centro, RadioDeteccion, 24, Color, false, -1.f, 0, 1.f);

		// Esfera del radio de ataque
		DrawDebugSphere(GetWorld(), Centro, RadioAtaque, 16, FColor::Orange, false, -1.f, 0, 1.f);
	}
}

ACharacter* UEnemigosComponent::GetPersonaje() const
{
	return Cast<ACharacter>(GetOwner());
}

// NUEVO
float UEnemigosComponent::GetPorcentajeVida() const
{
	// Si aún no ha empezado el juego (VidaInicial = 0), evitamos dividir entre 0
	return VidaInicial > 0 ? static_cast<float>(Vida) / static_cast<float>(VidaInicial) : 1.f;
}

void UEnemigosComponent::ActualizarVelocidad()
{
	if (ACharacter* Personaje = GetPersonaje())
	{
		if (UCharacterMovementComponent* Movimiento = Personaje->GetCharacterMovement())
		{
			Movimiento->MaxWalkSpeed = VelocidadMovimiento;
		}
	}
}

bool UEnemigosComponent::JugadorDentroDe(float Radio) const
{
	const AActor* Propietario = GetOwner();
	const APawn* Jugador = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Propietario || !Jugador)
	{
		return false;
	}

	// Comparación con distancia al cuadrado (más barato que usar sqrt)
	const float DistanciaSq = FVector::DistSquared(Propietario->GetActorLocation(), Jugador->GetActorLocation());
	return DistanciaSq <= FMath::Square(Radio);
}

bool UEnemigosComponent::JugadorEnRango() const
{
	return JugadorDentroDe(RadioDeteccion);
}

bool UEnemigosComponent::JugadorEnRangoAtaque() const
{
	return JugadorDentroDe(RadioAtaque);
}

bool UEnemigosComponent::IntentarAtacar()
{
	// 1. Tiene que estar vivo y con el jugador al alcance
	if (!EstaVivo() || !JugadorEnRangoAtaque())
	{
		return false;
	}

	// 2. Tiene que haber pasado el tiempo de espera desde el último ataque
	const UWorld* Mundo = GetWorld();
	if (!Mundo)
	{
		return false;
	}

	const float Ahora = Mundo->GetTimeSeconds();
	if (Ahora - TiempoUltimoAtaque < TiempoEntreAtaques)
	{
		return false;
	}

	// 3. Ataca y avisa al Blueprint para que reproduzca la animación
	TiempoUltimoAtaque = Ahora;

	APawn* Jugador = UGameplayStatics::GetPlayerPawn(this, 0);
	Atacar(Jugador);
	OnAtaque.Broadcast(Jugador);

	return true;
}

void UEnemigosComponent::ManejarDanioRecibido(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	RecibirDanio(FMath::RoundToInt(Damage));
}

void UEnemigosComponent::RecibirDanio(int32 Cantidad)
{
	if (!EstaVivo() || Cantidad <= 0)
	{
		return;
	}

	Vida = FMath::Max(0, Vida - Cantidad);
	UE_LOG(LogTemp, Log, TEXT("%s recibió %d de daño. Vida restante: %d"), *GetNameSafe(GetOwner()), Cantidad, Vida);

	OnDanioRecibido.Broadcast(Cantidad, Vida);

	if (!EstaVivo())
	{
		Morir();
	}
}

void UEnemigosComponent::Atacar(AActor* Objetivo)
{
	AActor* Propietario = GetOwner();
	if (!Objetivo || !Propietario || !EstaVivo())
	{
		return;
	}

	const APawn* PawnPropietario = Cast<APawn>(Propietario);
	AController* Controlador = PawnPropietario ? PawnPropietario->GetController() : nullptr;

	// Usa el sistema de daño de Unreal; el objetivo lo recibe en su TakeDamage
	UGameplayStatics::ApplyDamage(Objetivo, static_cast<float>(Danio), Controlador, Propietario, nullptr);
	UE_LOG(LogTemp, Log, TEXT("%s atacó a %s con %d de daño"), *Propietario->GetName(), *Objetivo->GetName(), Danio);
}

void UEnemigosComponent::Morir()
{
	if (bMuerto)
	{
		return;
	}
	bMuerto = true;

	AActor* Propietario = GetOwner();
	UE_LOG(LogTemp, Log, TEXT("%s ha muerto"), *GetNameSafe(Propietario));

	OnMuerte.Broadcast();

	if (ACharacter* Personaje = GetPersonaje())
	{
		Personaje->GetCharacterMovement()->DisableMovement();
	}

	if (Propietario)
	{
		Propietario->SetActorEnableCollision(false);
		Propietario->SetLifeSpan(2.f); // Se destruye a los 2 segundos (tiempo para animación/efectos)
	}

	SetComponentTickEnabled(false);
}