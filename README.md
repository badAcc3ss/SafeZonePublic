# SafeZone
This is an implementation of SafeZone in multiplayer gameplay using Unreal Engine

## Description
This implementation is designed to create a safe zone feature in a multiplayer environment where players will suffer from damage over time when they are outside the safezone. 

The implementation involves the creation of a safezone on the server as a sphere component that will shrink over time. The safezone component has a delay of 30 seconds and will shrink a maximum of 5 times. 

To create the safezone, it is divided into four quadrants, each represented by smaller sphere components actor inside the safezone. 

This approach allows for faster queries against the environment before using the safezone for further queries. 
The quadrants are queried to find the quadrant with the minimum number of players, and a random point in that quadrant is selected as a reference to shrink the safezone and its quadrants iteratively.

Client prediction methods using a replicated variable handle this process on both the server and client.
 
To handle character interactions, an Ability System Component and GamePlayAbility System are introduced, along with the PlayerHealth Attributes for health and maxhealth.
 
A DamageGameplayEffect is triggered later to reduce the health of the character. Overlap from the quadrants determines whether a player should be damaged or not.
 
When a player begins and ends interaction with a quadrant, it is checked whether it is valid to apply damage. 
  
If not, the player is left alone. The quadrant helps to iterate more quickly, and when a player leaves, they are first checked if they are still in a quadrant or not. 
  
If not found, a broader search in the safezone is conducted as there may be some edge case spots within the safezone where the quadrants don't overlap.   
If the character is still not found, a gameplay tag is added to the player, which triggers the execution of the DamageEffect to start damaging the player character's health. 
  
When a player runs out of health, death functions are executed. 
  
The implementation also includes the use of the Free Anim Pack for animation, which comes from assets included in the marketplace from Epic Games.

current with **Unreal Engine 4.26**. 

## Dependencies
This implementation requires the following dependencies:
### UE 4.26 or higher
### Visual Studio 2019


Classes and Functions

## SafeZoneGameState
This class provides the implementation of the maintaining player count for total number of players in game.

## SafeZoneGameMode
Except boiler code of PostLogin and Logout, contains code for managing, Adding and Removing player from SafeZone actor reference, more importantly used for Applying and Removing Damage tags from Player.

## SafeZoneActor
This class is an actor that actually manages the properties and quadrants of safe zone meanwhile also the shrinking and moving logic.

## QuadrantSystemActor
This class is an actor which is subclass of SafeZoneActor used for Player overlap interactions and query based system.

## PlayerAttributeSet
This class is an attribute class used for maintain the attribute of the charcter to be affected like health, damage.

## GamePlayerCharacter
This class is the main character class of game, used for character interaction and utilizing the GameplayAbilities and Effects via Ability System Component.

## DamageGE_ExecutionCalculation
This is damage calculation class that start apply damage to the health attribute of the character when applied as an effect.


### License
This implementation is licensed under the MIT License. See the LICENSE file for more information.
