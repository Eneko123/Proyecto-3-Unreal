// JugadorComponent.cpp

#include "JugadorComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h" // NUEVO: para usar los timers del dash
#include "DrawDebugHelpers.h"

UJugadorComponent::UJugadorComponent()
{
	// Este componente no necesita Tick: todo funciona por eventos y timers
	PrimaryComponentTick.bCanEverTick = false;

	// Combo de 3 golpes; el daño sube en cada golpe
	MultiplicadoresCombo = { 1.0f, 1.25f, 1.5f };
}

void UJugadorComponent::BeginPlay()
{
	Super::BeginPlay();

	// Empieza con la vida llena
	Vida = VidaMaxima;

	// Estado inicial del combo
	PasoCombo = 0;
	DanioAtaqueActual = Danio;

	// NUEVO: estado inicial del dash
	bHaciendoDash = false;
	bPuedeDash = true;
	bDashAereoUsado = false;

	if (AActor* Propietario = GetOwner())
	{
		// El daño aplicado al jugador (ApplyDamage, por ejemplo desde el enemigo) llega aquí
		Propietario->OnTakeAnyDamage.AddDynamic(this, &UJugadorComponent::ManejarDanioRecibido);
	}

	// NUEVO: cuando el personaje aterriza, recupera el dash aéreo (lo que haría "Event On Landed" en Blueprint)
	if (ACharacter* Personaje = GetPersonaje())
	{
		Personaje->LandedDelegate.AddDynamic(this, &UJugadorComponent::ManejarAterrizaje);
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

int32 UJugadorComponent::GetNumGolpesCombo() const
{
	return FMath::Max(1, MultiplicadoresCombo.Num());
}

void UJugadorComponent::ReiniciarCombo()
{
	PasoCombo = 0;
}

// CAMBIADO: no deja atacar durante el dash
bool UJugadorComponent::IntentarAtacar()
{
	if (!EstaVivo())
	{
		return false;
	}

	// NUEVO: durante el dash no se ataca (equivale a "Can Attack = false" del Blueprint)
	if (bHaciendoDash)
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
	const float TiempoDesdeUltimo = Ahora - TiempoUltimoAtaque;
	if (TiempoDesdeUltimo < TiempoEntreAtaques)
	{
		return false;
	}

	// Si tardaste demasiado o ya hiciste el último golpe, el combo empieza de nuevo
	const int32 NumGolpes = GetNumGolpesCombo();
	if (TiempoDesdeUltimo > VentanaCombo || PasoCombo >= NumGolpes)
	{
		PasoCombo = 0;
	}

	TiempoUltimoAtaque = Ahora;

	// Siguiente golpe del combo y su daño
	++PasoCombo;
	const bool bEsGolpeFinal = PasoCombo >= NumGolpes;
	const float Multiplicador = MultiplicadoresCombo.IsValidIndex(PasoCombo - 1) ? MultiplicadoresCombo[PasoCombo - 1] : 1.f;
	DanioAtaqueActual = FMath::RoundToInt(static_cast<float>(Danio) * Multiplicador);

	UE_LOG(LogTemp, Log, TEXT("Golpe %d/%d del combo (%d de daño)"), PasoCombo, NumGolpes, DanioAtaqueActual);

	// Avisa al Blueprint (animación) y aplica el golpe
	OnAtaque.Broadcast(PasoCombo, bEsGolpeFinal);
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

		// Usa el daño del paso actual del combo
		UGameplayStatics::ApplyDamage(Actor, static_cast<float>(DanioAtaqueActual), Controlador, Propietario, nullptr);
		UE_LOG(LogTemp, Log, TEXT("El jugador golpeó a %s con %d de daño"), *Actor->GetName(), DanioAtaqueActual);
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

// NUEVO: equivale a la función "Dash" del Blueprint de Eneko
bool UJugadorComponent::IntentarDash()
{
	// 1. ¿Se puede hacer dash? (vivo, sin cooldown y sin otro dash en marcha)
	if (!EstaVivo() || !bPuedeDash || bHaciendoDash)
	{
		return false;
	}

	ACharacter* Personaje = GetPersonaje();
	UWorld* Mundo = GetWorld();
	if (!Personaje || !Mundo)
	{
		return false;
	}

	UCharacterMovementComponent* Movimiento = Personaje->GetCharacterMovement();
	if (!Movimiento)
	{
		return false;
	}

	// Branch "Is Falling AND Air Dash Used": en el aire solo hay un dash por salto
	const bool bEnElAire = Movimiento->IsFalling();
	if (bEnElAire && bDashAereoUsado)
	{
		return false;
	}

	// 2. Bloquea estados (los SET de Can Dash, Air Dash Used e Is Dashing)
	bPuedeDash = false;
	bHaciendoDash = true;
	if (bEnElAire)
	{
		// Solo gasta el dash aéreo si de verdad estaba en el aire
		bDashAereoUsado = true;
	}

	// El dash corta el combo
	ReiniciarCombo();

	// 3. Guarda la velocidad vertical: si estaba cayendo, la conserva; si no, usa VelocidadZTrasDash
	const float VelocidadZActual = Movimiento->Velocity.Z;
	VelocidadZGuardada = (VelocidadZActual < 0.f) ? VelocidadZActual : VelocidadZTrasDash;

	// 4. Quita la gravedad durante el dash, guardando la que tenía
	GravedadOriginal = Movimiento->GravityScale;
	Movimiento->GravityScale = 0.f;

	// 5. Dirección: la del input; si no hay input, hacia donde mira el personaje
	FVector Direccion = Personaje->GetLastMovementInputVector();
	Direccion.Z = 0.f;
	if (Direccion.IsNearlyZero(0.1f))
	{
		Direccion = Personaje->GetActorForwardVector();
	}
	// Normalizada: con mando, el stick a medias no hace el dash más corto
	Direccion = Direccion.GetSafeNormal2D();

	// Velocidad = distancia / tiempo, así recorre DistanciaDash en DuracionDash segundos
	const float Duracion = FMath::Max(DuracionDash, 0.01f);
	const float Velocidad = DistanciaDash / Duracion;

	// Launch Character con XY Override y Z Override marcados
	Personaje->LaunchCharacter(Direccion * Velocidad, true, true);

	// 6. Timers: fin del dash y fin del cooldown
	FTimerManager& Timers = Mundo->GetTimerManager();
	Timers.SetTimer(TimerFinDash, this, &UJugadorComponent::TerminarDash, Duracion, false);

	if (CooldownDash > 0.f)
	{
		Timers.SetTimer(TimerCooldownDash, this, &UJugadorComponent::ReiniciarCooldownDash, CooldownDash, false);
	}
	else
	{
		// Sin cooldown: bHaciendoDash ya impide encadenar otro dash hasta que acabe este
		bPuedeDash = true;
	}

	UE_LOG(LogTemp, Log, TEXT("Dash %s (velocidad %.0f)"), bEnElAire ? TEXT("aéreo") : TEXT("en suelo"), Velocidad);

	OnDash.Broadcast();
	return true;
}

// NUEVO: equivale al evento "EndDash" del Blueprint
void UJugadorComponent::TerminarDash()
{
	if (!bHaciendoDash)
	{
		return;
	}
	bHaciendoDash = false;

	// Por si se llama antes de tiempo, cancela el timer pendiente
	if (UWorld* Mundo = GetWorld())
	{
		Mundo->GetTimerManager().ClearTimer(TimerFinDash);
	}

	if (ACharacter* Personaje = GetPersonaje())
	{
		if (UCharacterMovementComponent* Movimiento = Personaje->GetCharacterMovement())
		{
			// Devuelve la gravedad que tenía (no un 1.0 fijo)
			Movimiento->GravityScale = GravedadOriginal;

			// Corta el impulso horizontal y recupera la velocidad vertical guardada
			Movimiento->Velocity = FVector(0.f, 0.f, VelocidadZGuardada);
		}
	}

	OnFinDash.Broadcast();
}

// NUEVO: equivale al evento "ResetDash" del Blueprint
void UJugadorComponent::ReiniciarCooldownDash()
{
	bPuedeDash = true;
}

// NUEVO: al tocar el suelo vuelve a tener dash aéreo
void UJugadorComponent::ManejarAterrizaje(const FHitResult& Hit)
{
	bDashAereoUsado = false;
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

	ReiniciarCombo();

	// NUEVO: si muere a mitad de dash, lo termina (restaura gravedad) y cancela el cooldown
	TerminarDash(); // NUEVO
	if (UWorld* Mundo = GetWorld()) // NUEVO
	{
		Mundo->GetTimerManager().ClearTimer(TimerCooldownDash); // NUEVO
	}

	OnMuerte.Broadcast();

	if (ACharacter* Personaje = GetPersonaje())
	{
		// Deja de moverse y de responder a los controles (no se destruye)
		Personaje->GetCharacterMovement()->DisableMovement();
		Personaje->DisableInput(Cast<APlayerController>(Personaje->GetController()));
	}
}