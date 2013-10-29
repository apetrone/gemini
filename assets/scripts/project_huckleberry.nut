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


enum CollisionGroup
{
	NONE,
	COW,
	MARTIAN,
	PLAYER,
	PROJECTILE,
	SENSOR
}

enum MartianState
{
	INVALID,
	IDLE,
	SEEKING,
	FOUND_TARGET,
	ABDUCTING,
	FINISHED_ABDUCTION,
	ESCAPE_WITH_ABDUCTEE
}





class Martian extends SpriteEntity
{
	target_cow			= null
	sound_abduction		= null
	target_delta		= null

	state 				= 0
	level				= 0
	advance 			= 0
	direction			= 1
	side				= 0

	start_origin 		= null
	constructor()
	{
		base.constructor()

		// -render.width() * 3
		
		start_origin = vec2( -this.width(), 50 )
		local start_velocity = vec2( 120, 0)
		set_sprite( "sprites/martian" )
		this.position = start_origin
		this.velocity = start_velocity
		
		level = 0
		state = MartianState.SEEKING
		this.advance = this.height() * 1.5

		// state = MartianState.IDLE
		// this.position = vec2( 150, 50 )
	}

	function step( delta_seconds )
	{
		this.position = this.position + (this.velocity * delta_seconds)
		// if ( this.position.x > (render.width() - (this.width()/2.0)) )
		// {
		// 	this.position = vec2( start_origin.x, this.position.y + this.advance )
		// 	level++
		// }

		base.step( delta_seconds )
	}

	function tick()
	{
		local barrier_size = (render.width() * 2)
		local right_barrier = render.width() + this.width()
		local left_barrier = -this.width()
		
		local position = this.position
		local velocity = this.velocity

		if (level == 0)
		{
			this.set_color( 255, 128, 128, 255 )
		}
		else if (level == 1)
		{
			this.set_color( 128, 255, 128, 255 )
		}
		else if (level == 2)
		{
			this.set_color( 128, 128, 255, 255 )
		}
		else if (level == 3)
		{
			this.set_color( 255, 215, 0, 255 )
		}

		if (state == MartianState.SEEKING)
		{
			if ( position.x > right_barrier || position.x < left_barrier )
			{
				velocity.x = -velocity.x
				direction = -direction
				position.y += advance
				if ( position.x > right_barrier && velocity.x > 0 )
				{
					position.x = barrier_size
				}
				else if ( position.x < left_barrier && velocity.x < 0 )
				{
					position.x = left_barrier
				}

				// update position
				this.position = position

				level++
			}
		}
		else if (state == MartianState.FOUND_TARGET)
		{
			// try to align with the target first
			velocity.x *= 0.98
			local target_pos = target.position
			this.target_delta = target_pos - this.position
			if ( abs(target_delta.x) < 2 )
			{
				// aligned with the target, now abduct it!
				state = MartianState.ABDUCTING
				target.state = CowState.BEING_ABDUCTED
				print( "TODO: play abduction ray sound\n" )
				//abduction_ray_sound = audio.play( abduction_ray, 1 )
			}
		}
		else if (state == MartianState.ABDUCTING)
		{
			velocity.x = 0
			state = MartianState.FINISHED_ABDUCTION
		}
		else if (state == MartianState.FINISHED_ABDUCTION)
		{
			print( "TODO: stop audio playback\n" )
			// audio.stop( abduction_ray_sound )
			// abduction_ray_sound = -1
			state = MartianState.ESCAPE_WITH_ABDUCTEE
			velocity.y = -5
		}
		else if (state == MartianState.ESCAPE_WITH_ABDUCTEE)
		{
			velocity.y -= 0.25
			velocity.x += 0.3 * direction
			local target_pos = this.position + target_delta
			target.position = target_pos
		}

		if ( state == MartianState.IDLE )
		{
			velocity.x = 0
			velocity.y = 0
		}
		this.velocity = velocity
		base.tick()
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
	state 				= null
	bounds				= []
	side 				= 0

	constructor()
	{
		base.constructor()
		state = CowState.GRAZING
		set_sprite( "sprites/cow" )
		// this.collision_group = CollisionGroup.COW
		// this.collision_mask = CollisionGroup.SENSOR

	}

	function step( delta_seconds )
	{
		base.step( delta_seconds )
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

class Projectile extends SpriteEntity
{

}


class Firebird extends GameRules
{
	next_spawn = 0
	martians = []
	projectiles = []

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
			//print( "Spawn a new martian\n" )
			local m = Martian()
			martians.push( m )
		}
	}

	function tick()
	{
		for (local i = 0; i < martians.len(); ++i)
		{
			local m = martians[i]
			if (m.level >= 4)
			{
				martians.remove(i)
			}
		}

		for(local i = 0; i < projectiles.len(); ++i)
		{
			local p = projectiles[i]
			if (p.position.x > render.width() || p.position.x < 0 || p.position.y < 0 || p.position.y > render.height())
			{
				print( "remove this projectile\n" )
			}
		}
	}
}

// gamerules <- HuckleberryRules()
gamerules <- Firebird()