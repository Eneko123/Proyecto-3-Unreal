// JugadorComponent.h
// Componente de vida y ataque para el jugador (Unreal Engine 5.8.2)
// Se añade al Blueprint del jugador desde el panel "Components".

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JugadorComponent.generated.h"

class ACharacter;
class AController;
class UDamageType;

// Eventos para que el Blueprint del jugador reaccione (animaciones, HUD, sonidos...)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnJugadorDanio, int32, Cantidad, int32, VidaRestante);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnJugadorMuerte);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnJugadorAtaque);

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

	// Dibuja la zona del golpe durante 1 segundo para depurar
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jugador|Debug")
	bool bMostrarGolpe = false;

	// ---------- Eventos ----------

	UPROPERTY(BlueprintAssignable, Category = "Jugador|Eventos")
	FOnJugadorDanio OnDanioRecibido;

	UPROPERTY(BlueprintAssignable, Category = "Jugador|Eventos")
	FOnJugadorMuerte OnMuerte;

	// Se lanza cuando el jugador ataca: úsalo para reproducir la animación
	UPROPERTY(BlueprintAssignable, Category = "Jugador|Eventos")
	FOnJugadorAtaque OnAtaque;

	// ---------- Funciones ----------

	// Ataca si está vivo y ya pasó TiempoEntreAtaques. Devuelve true si atacó.
	UFUNCTION(BlueprintCallable, Category = "Jugador|Ataque")
	bool IntentarAtacar();

	// Lanza el golpe hacia delante y hace daño a los Pawns que toque. Devuelve cuántos golpeó.
	UFUNCTION(BlueprintCallable, Category = "Jugador|Ataque")
	int32 AplicarGolpe();

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

private:
	bool bMuerto = false;

	// Momento (en segundos de juego) del último ataque
	float TiempoUltimoAtaque = -1000.f;
};