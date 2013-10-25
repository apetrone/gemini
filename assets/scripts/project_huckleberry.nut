// class RotatingModel extends ModelEntity
// {
// 	function tick()
// 	{
// 		local delta_angle = 0.5

// 		// rotate around the Y-axis
// 		this.transform = rotate( this.transform, delta_angle, vec3(0, 1, 0) )

// 		// debug.axes( this.transform, 1.0, 0.0 )
// 	}
// }


// class PlayerControl extends ModelEntity
// {
// 	function tick()
// 	{
		
// 	}
// }

// ground <- ModelEntity()
// ground.name = "ground"
// ground.set_model( "models/room2")

// vehicle <- RotatingModel()
// vehicle.name = "Vehicle"
// vehicle.transform = translate( vehicle.transform, vec3(0, 0.3432, 0.0) )
// vehicle.set_model( "models/vehicle" )


local ENEMY_LAYER = 1


class Player extends SpriteEntity
{
	smoke_puff = null
	ticks_to_remove = 120
	smoke_offset = vec2(32, -5)

	constructor()
	{
		base.constructor()

		smoke_puff = EmitterEntity( this )
		smoke_puff.set_emitter( "sprites/exhaust" )

		this.layer = 0
	}

	function tick()
	{
		base.tick()

		if ( smoke_puff && (ticks_to_remove > 0) )
		{
			// ticks_to_remove--
		}

		if ( ticks_to_remove == 0 )
		{
			ticks_to_remove = -1
			print( "removing smoke puff\n" )
			smoke_puff = null
		}
	}

	function step( delta_seconds )
	{
		base.step( delta_seconds )
		//this.position = this.position + vec2( 35 * delta_seconds, 0 )

		if ( smoke_puff )
		{
			smoke_puff.position = this.position + smoke_offset
		}
	}
}


class Enemy extends SpriteEntity
{
	constructor()
	{
		base.constructor()
		this.layer = ENEMY_LAYER
	}
}




// enemy <- Enemy()
// enemy.set_sprite( "sprites/enemy" )
// enemy.position = vec2( 120, 100 )


class HuckleberryRules extends GameRules
{
	player = null
	// cows = []
	function startup()
	{
		player = Player()
		player.position = vec2( 150, 100 )
		player.set_sprite( "sprites/player" )

		// local cow = Cow()
		// cows.push( cow )
	}

 	function tick()
	{
		//print( "HuckleBerry tick\n" )
	}

	function step( delta_seconds )
	{
		//print( delta_seconds + "\n" )
	}

	function click( id, x, y )
	{
		print( "click\n" )
	}
}




enum MartianStates
{
	MARTIAN_INVALID,
	MARTIAN_SEEKING,
	MARTIAN_FOUND_TARGET,
	MARTIAN_ABDUCTING,
	MARTIAN_FINISHED_ABDUCTION,
	MARTIAN_ESCAPE_WITH_ABDUCTEE
}





class Martian extends SpriteEntity
{
	target_cow			= null
	sound_abduction		= null
	target_delta		= null

	level				= 0
	advance 			= vec2(0, 60)
	direction			= 1
	side				= 0

	constructor()
	{
		base.constructor()

		// -render.width() * 3
		local start_origin = vec2( 50, 50 )
		local start_velocity = vec2(120, 0)

		this.position = start_origin
		this.velocity = start_velocity
		set_sprite( "sprites/martian" )
	}
}




// states
enum CowState
{
	GRAZING,
	BEING_ABDUCTED,
	FALLING,
	ABDUCTION_COOLDOWN,
}

class Cow extends SpriteEntity
{
	direction 			= 0
	state 				= CowState.GRAZING
	next_think			= 0
	bounds				= []

	constructor()
	{
		base.constructor()
		set_sprite( "sprites/cow" )
	}

	function step( delta_seconds )
	{

	}

	function tick()
	{
		local vel = this.velocity
		local pos = this.position
		if ( state == CowState.GRAZING )
		{
			vel.x = direction * 20.0
			vel.y = 0
		}
		else if ( state == CowState.BEING_ABDUCTED )
		{
			vel.x = 0
		}
		else if ( state == CowState.FALLING )
		{
			vel.y += 9.8
		}
		else if ( state == CowState.ABDUCTION_COOLDOWN )
		{
			state = CowState.GRAZING
		}

		this.velocity = vel
	}
}


class Firebird extends GameRules
{
	next_spawn = 0
	martians = []

	function startup()
	{
		// nothing here
		print( "Width: " + render.width() + "\n" )
		print( "Height: " + render.height() + "\n" )
	}

	function step( delta_seconds )
	{
		next_spawn -= delta_seconds
		if ( next_spawn <= 0 )
		{
			next_spawn = 1
			print( "Spawn a new martian\n" )
			local m = Martian()
			martians.push( m )
		}
	}
}

// gamerules <- HuckleberryRules()
gamerules <- Firebird()