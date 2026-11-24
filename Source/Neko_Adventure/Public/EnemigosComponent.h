// EnemigosComponent.h
// Componente base para todos los enemigos del proyecto (Unreal Engine 5.8.2)
// Se añade a un Character desde el panel "Components" del Blueprint.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemigosComponent.generated.h"

class ACharacter;
class AController;
class UDamageType;

// Eventos para que el Blueprint del enemigo reaccione (animaciones, sonidos, UI...)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnemigoDanio, int32, Cantidad, int32, VidaRestante);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnemigoMuerte);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemigoAtaque, AActor*, Objetivo);

UCLASS(ClassGroup = (Enemigos), Blueprintable, meta = (BlueprintSpawnableComponent))
class NEKO_ADVENTURE_API UEnemigosComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemigosComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ---------- Stats base que heredan todos los enemigos ----------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemigo|Stats", meta = (ClampMin = "0"))
	int32 Vida = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemigo|Stats", meta = (ClampMin = "0"))
	int32 Danio = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemigo|Stats", meta = (ClampMin = "0.0", Units = "cm"))
	float RadioDeteccion = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemigo|Stats", meta = (ClampMin = "0.0", Units = "CentimetersPerSecond"))
	float VelocidadMovimiento = 300.f;

	// ---------- Ataque ----------

	// Distancia a la que el enemigo puede golpear al jugador
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemigo|Ataque", meta = (ClampMin = "0.0", Units = "cm"))
	float RadioAtaque = 150.f;

	// Segundos de espera entre un ataque y el siguiente
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemigo|Ataque", meta = (ClampMin = "0.0", Units = "s"))
	float TiempoEntreAtaques = 1.5f;

	// Dibuja las esferas de detección (verde/roja) y de ataque (naranja) para depurar
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemigo|Debug")
	bool bMostrarRadioDeteccion = false;

	// ---------- Eventos ----------

	UPROPERTY(BlueprintAssignable, Category = "Enemigo|Eventos")
	FOnEnemigoDanio OnDanioRecibido;

	UPROPERTY(BlueprintAssignable, Category = "Enemigo|Eventos")
	FOnEnemigoMuerte OnMuerte;

	// Se lanza cada vez que el enemigo ataca: úsalo en el Blueprint para reproducir la animación
	UPROPERTY(BlueprintAssignable, Category = "Enemigo|Eventos")
	FOnEnemigoAtaque OnAtaque;

	// ---------- Funciones comunes (los hijos pueden sobrescribirlas) ----------

	UFUNCTION(BlueprintCallable, Category = "Enemigo")
	virtual void RecibirDanio(int32 Cantidad);

	UFUNCTION(BlueprintCallable, Category = "Enemigo")
	virtual void Atacar(AActor* Objetivo);

	UFUNCTION(BlueprintCallable, Category = "Enemigo")
	virtual void Morir();

	UFUNCTION(BlueprintPure, Category = "Enemigo")
	bool EstaVivo() const { return Vida > 0; }

	// Vida actual entre 0 y 1 (útil para una barra de vida) // NUEVO
	UFUNCTION(BlueprintPure, Category = "Enemigo") // NUEVO
		float GetPorcentajeVida() const; // NUEVO

	UFUNCTION(BlueprintPure, Category = "Enemigo")
	bool JugadorEnRango() const;

	// True si el jugador está dentro de RadioAtaque
	UFUNCTION(BlueprintPure, Category = "Enemigo|Ataque")
	bool JugadorEnRangoAtaque() const;

	// Ataca al jugador si está en rango de ataque y ya pasó TiempoEntreAtaques. Devuelve true si atacó.
	UFUNCTION(BlueprintCallable, Category = "Enemigo|Ataque")
	bool IntentarAtacar();

	// Aplica VelocidadMovimiento al CharacterMovement del dueño (llámala si cambias la velocidad en runtime)
	UFUNCTION(BlueprintCallable, Category = "Enemigo")
	void ActualizarVelocidad();

	// Devuelve el Character que tiene este componente (nullptr si el dueño no es un Character)
	UFUNCTION(BlueprintPure, Category = "Enemigo")
	ACharacter* GetPersonaje() const;

protected:
	virtual void BeginPlay() override;

	// Se llama también en el editor: aplica la velocidad al colocar/editar el actor
	virtual void OnRegister() override;

	// Recibe el daño que llega al actor dueño mediante ApplyDamage
	UFUNCTION()
	void ManejarDanioRecibido(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

private:
	// Comprueba si el jugador está a menos de Radio cm del dueño
	bool JugadorDentroDe(float Radio) const;

	bool bMuerto = false;

	// Vida con la que empezó el enemigo (se guarda en BeginPlay) // NUEVO
	int32 VidaInicial = 0; // NUEVO

	// Momento (en segundos de juego) del último ataque
	float TiempoUltimoAtaque = -1000.f;
};