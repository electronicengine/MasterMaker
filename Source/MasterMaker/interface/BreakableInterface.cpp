// Fill out your copyright notice in the Description page of Project Settings.


#include "BreakableInterface.h"
#include "PlugInterface.h"
#include "../brick/MechanicBrick.h"
#include "../brick/Brick.h"
#include "../brick/DestructedBrick.h"
#include "../vehicle/VehicleBase.h"
#include "../character/EnemyCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "../character/UserPlayerState.h"

IBreakableInterface::IBreakableInterface()
{
    Breaked_ = false;
}

bool IBreakableInterface::addDamage(float Value, const FVector& ImpactPoint, const FVector& Velocity)
{
    bool ret = false;



    if (this && !Breaked_) {
        FVector Origin;
        FVector BoxExtent;
        float SphereRadius;

        UKismetSystemLibrary::GetComponentBounds(Cast<AActor>(this)->GetRootComponent(), Origin, BoxExtent, SphereRadius);
        if ((ImpactPoint - Origin).Size() <= SphereRadius || Cast<AEnemyCharacter>(this)) {
            ret = addDamageToSelf(Value, ImpactPoint, Velocity);
        }
        else {
                
            addDamageToPlugins(Value, ImpactPoint, Velocity);
        }
    }

    return ret;
}




void IBreakableInterface::addDamageToPlugins(float Value, const FVector& ImpactPoint, const FVector& Velocity)
{
    
    if (Cast<IPlugInterface>(this)) {

        std::map<int, ABrick*>& plugins = Cast<IPlugInterface>(this)->Plugged_Items_OnIt;
        std::map<int, ABrick*>::iterator it;

        for (it = plugins.begin(); it != plugins.end();)
        {
            if (it->second == nullptr)
                break;

            bool res = it->second->addDamage(Value, ImpactPoint, Velocity);
            if (!res) {
                ++it;
            }
            else {
                break;
            }
       
        }
    }
}

bool IBreakableInterface::addDamageToSelf(float Value, const FVector& ImpactPoint, const FVector& Velocity)
{
    bool ret = false;
    
    if (Cast<AVehicleBase>(this)){
        if (Cast<AVehicleBase>(this)->Plugged_Items_OnIt.size() != 0) {
            addDamageToPlugins(Value, ImpactPoint, Velocity);
            return false;
        }
    }

    if (Cast<IPlugInterface>(this)) {
        std::map<int, ABrick*>& plugged_items_onit = Cast<IPlugInterface>(this)->Plugged_Items_OnIt;
    }

    Healt_ -= Value;

    if (Healt_ <= 0 && !Breaked_) {

        if (Cast<AVehicleBase>(this)) {
            Cast<AVehicleBase>(this)->exitCar();
        }

        breakItem(ImpactPoint, Velocity);
        ret = true;
    }

    return ret;
}


void IBreakableInterface::breakItem(const FVector& ImpactPoint, const FVector& Velocity)
{

    if (Cast<AMechanicBrick>(this))
        return;

    FVector spawn_location = Cast<AActor>(this)->GetActorLocation();
    FRotator spawn_rotation = Cast<AActor>(this)->GetActorRotation();
    Breaked_ = true;

    if (Cast<IPlugInterface>(this)) {
        Cast<IPlugInterface>(this)->notifyToOwnerItemDestroyed(Cast<IPlugInterface>(this)->Item_Index);

        Cast<IPlugInterface>(this)->detachItemsOnIt();

    }

    if(Cast<AActor>(this))
        Cast<AActor>(this)->Destroy();

    UWorld* world = Cast<AActor>(this)->GetWorld();
    APlayerController* controller = UGameplayStatics::GetPlayerController(world, 0);
    AUserPlayerState* player_state = Cast<AUserPlayerState>(controller->GetPawn()->GetPlayerState());
    player_state->destroyedBrick(Destroyed_Id);

    ADestructedBrick* dest_ptr;

    dest_ptr = world->SpawnActor<ADestructedBrick>(ADestructedBrick::StaticClass(), spawn_location, spawn_rotation);
    dest_ptr->setMesh(Item_Name);
    dest_ptr->setColor(Item_Color);

    dest_ptr->setImpactPointAndVelocity(ImpactPoint, Velocity);

    const FDetachmentTransformRules& detachment_rules = FDetachmentTransformRules(EDetachmentRule::KeepWorld,
        EDetachmentRule::KeepWorld,
        EDetachmentRule::KeepWorld, true);

}

void IBreakableInterface::setId(int Id)
{
    Destroyed_Id = Id;
    if (Cast<IPlugInterface>(this)) {
        Cast<IPlugInterface>(this)->Id_ = Id;
    }
}


