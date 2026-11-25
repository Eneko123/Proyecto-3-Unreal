// JugadorComponent.h
// Componente de vida, ataque con combo y dash para el jugador (Unreal Engine 5.8.2)
// Se añade al Blueprint del jugador desde el panel "Components".

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/TimerHandle.h" // NUEVO: para los timers del dash
#include "JugadorComponent.generated.h"

class ACharacter;
class AController;
class UDamageType;

// Eventos para que el Blueprint del jugador reaccione (animaciones, HUD, sonidos...)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnJugadorDanio, int32, Cantidad, int32, VidaRestante);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnJugadorMuerte);
// CAMBIADO: ahora dice qué golpe del combo es (1, 2, 3...) y si es el último
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnJugadorAtaque, int32, PasoCombo, bool, bEsGolpeFinal);
// NUEVO: avisan cuando empieza y cuando termina el dash
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnJugadorDash); // NUEVO
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnJugadorFinDash); // NUEVO

UCLASS(ClassGroup = (Jugador), Blueprintable, meta = (BlueprintSpawnableComponent))
class NEKO_ADVENTURE_API UJugadorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJugadorComponent();

	// ---------- Vida ----------

	// Vida con la que empieza el jugador
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jugador|Vida", meta = (ClampMin = "1"))
	int32 VidaMaxima = 100;

	// Vida actual (se rellena con VidaMaxima al empezar)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jugador|Vida")
	int32 Vida = 100;

	// ---------- Ataque ----------

	// Daño base de cada golpe (se multiplica según el paso del combo)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jugador|Ataque", meta = (ClampMin = "0"))
	int32 Danio = 25;

	// Hasta dónde llega el golpe hacia delante
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jugador|Ataque", meta = (ClampMin = "0.0", Units = "cm"))
	float AlcanceAtaque = 150.f;

	// Grosor del golpe (radio de la esfera que se lanza hacia delante)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jugador|Ataque", meta = (ClampMin = "0.0", Units = "cm"))
	float RadioGolpe = 60.f;

	// Segundos de espera entre un ataque y el siguiente
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jugador|Ataque", meta = (ClampMin = "0.0", Units = "s"))
	float TiempoEntreAtaques = 0.6f;

	// ---------- Combo ----------

	// Multiplicador de daño de cada golpe del combo. Hay tantos golpes como elementos (por defecto 3).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jugador|Combo")
	TArray<float> MultiplicadoresCombo;

	// Segundos máximos entre golpes para seguir el combo. Tiene que ser MAYOR que TiempoEntreAtaques.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jugador|Combo", meta = (ClampMin = "0.0", Units = "s"))
	float VentanaCombo = 1.2f;

	// Golpe actual del combo: 0 = sin combo, 1 = primer golpe, 2 = segundo...
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Jugador|Combo")
	int32 PasoCombo = 0;

	// Daño que hace el golpe actual (Danio x multiplicador)
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Jugador|Combo")
	int32 DanioAtaqueActual = 0;

	// ---------- Dash ---------- // NUEVO

	// Distancia que recorre el dash (equivale a "Dash Distance" del Blueprint)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jugador|Dash", meta = (ClampMin = "0.0", Units = "cm")) // NUEVO
		float DistanciaDash = 600.f; // NUEVO

	// Cuánto dura el dash (equivale a "Dash Time"). Velocidad = DistanciaDash / DuracionDash
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jugador|Dash", meta = (ClampMin = "0.01", Units = "s")) // NUEVO
		float DuracionDash = 0.2f; // NUEVO

	// Segundos hasta poder volver a hacer dash, contados desde que EMPIEZA (equivale a "Dash Cooldown")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jugador|Dash", meta = (ClampMin = "0.0", Units = "s")) // NUEVO
		float CooldownDash = 0.8f; // NUEVO

	// Velocidad vertical al terminar si no estabas cayendo (el -200 del Blueprint): un pequeño empujón hacia abajo
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jugador|Dash", meta = (Units = "cm/s")) // NUEVO
		float VelocidadZTrasDash = -200.f; // NUEVO

	// true mientras dura el dash (equivale a "Is Dashing")
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Jugador|Dash") // NUEVO
		bool bHaciendoDash = false; // NUEVO

	// false durante el cooldown (equivale a "Can Dash")
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Jugador|Dash") // NUEVO
		bool bPuedeDash = true; // NUEVO

	// true si ya gastaste el dash en este salto; se pone a false al aterrizar (equivale a "Air Dash Used")
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Jugador|Dash") // NUEVO
		bool bDashAereoUsado = false; // NUEVO

	// ---------- Debug ----------

	// Dibuja la zona del golpe durante 1 segundo para depurar
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jugador|Debug")
	bool bMostrarGolpe = false;

	// ---------- Eventos ----------

	UPROPERTY(BlueprintAssignable, Category = "Jugador|Eventos")
	FOnJugadorDanio OnDanioRecibido;

	UPROPERTY(BlueprintAssignable, Category = "Jugador|Eventos")
	FOnJugadorMuerte OnMuerte;

	// Se lanza en cada golpe: úsalo para elegir la animación según el paso del combo
	UPROPERTY(BlueprintAssignable, Category = "Jugador|Eventos")
	FOnJugadorAtaque OnAtaque;

	// Se lanza al empezar el dash: úsalo para la animación, partículas, sonido...
	UPROPERTY(BlueprintAssignable, Category = "Jugador|Eventos") // NUEVO
		FOnJugadorDash OnDash; // NUEVO

	// Se lanza al terminar el dash
	UPROPERTY(BlueprintAssignable, Category = "Jugador|Eventos") // NUEVO
		FOnJugadorFinDash OnFinDash; // NUEVO

	// ---------- Funciones ----------

	// CAMBIADO: hace el siguiente golpe del combo si ya pasó TiempoEntreAtaques y no estás en dash. Devuelve true si atacó.
	UFUNCTION(BlueprintCallable, Category = "Jugador|Ataque")
	bool IntentarAtacar();

	// Lanza el golpe hacia delante y hace daño a los Pawns que toque. Devuelve cuántos golpeó.
	UFUNCTION(BlueprintCallable, Category = "Jugador|Ataque")
	int32 AplicarGolpe();

	// Vuelve el combo al primer golpe (por ejemplo, al recibir daño o al hacer dash)
	UFUNCTION(BlueprintCallable, Category = "Jugador|Combo")
	void ReiniciarCombo();

	// Cuántos golpes tiene el combo
	UFUNCTION(BlueprintPure, Category = "Jugador|Combo")
	int32 GetNumGolpesCombo() const;

	// Hace el dash si se puede (equivale a la función "Dash" del Blueprint). Devuelve true si lo hizo.
	UFUNCTION(BlueprintCallable, Category = "Jugador|Dash") // NUEVO
		bool IntentarDash(); // NUEVO

	// Termina el dash (lo llama el timer, pero también sirve para cortarlo antes, por ejemplo al chocar)
	UFUNCTION(BlueprintCallable, Category = "Jugador|Dash") // NUEVO
		void TerminarDash(); // NUEVO

	UFUNCTION(BlueprintCallable, Category = "Jugador|Vida")
	virtual void RecibirDanio(int32 Cantidad);

	UFUNCTION(BlueprintCallable, Category = "Jugador|Vida")
	virtual void Morir();

	UFUNCTION(BlueprintPure, Category = "Jugador|Vida")
	bool EstaVivo() const { return Vida > 0; }

	// Vida actual entre 0 y 1 (útil para una barra de vida)
	UFUNCTION(BlueprintPure, Category = "Jugador|Vida")
	float GetPorcentajeVida() const;

	// Devuelve el Character que tiene este componente (nullptr si el dueño no es un Character)
	UFUNCTION(BlueprintPure, Category = "Jugador")
	ACharacter* GetPersonaje() const;

protected:
	virtual void BeginPlay() override;

	// Recibe el daño que llega al actor dueño mediante ApplyDamage
	UFUNCTION()
	void ManejarDanioRecibido(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	// NUEVO: se llama cuando el personaje toca el suelo después de estar en el aire (recupera el dash aéreo)
	UFUNCTION() // NUEVO
		void ManejarAterrizaje(const FHitResult& Hit); // NUEVO

	// NUEVO: lo llama el timer del cooldown (equivale a "ResetDash")
	void ReiniciarCooldownDash(); // NUEVO

private:
	bool bMuerto = false;

	// Momento (en segundos de juego) del último ataque
	float TiempoUltimoAtaque = -1000.f;

	// NUEVO: velocidad vertical que se recupera al terminar el dash (equivale a "Dash Saved Z")
	float VelocidadZGuardada = 0.f; // NUEVO

	// NUEVO: gravedad que tenía el personaje antes del dash, para devolverle la misma
	float GravedadOriginal = 1.f; // NUEVO

	// NUEVO: timers del dash (equivalen a los "Set Timer by Event" del Blueprint)
	FTimerHandle TimerFinDash; // NUEVO
	FTimerHandle TimerCooldownDash; // NUEVO
};