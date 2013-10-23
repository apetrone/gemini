print( "Hello from squirrel!\n" )

function test()
{
	print( "setting up game\n" )


	local e = Entity()

	e.name = "Vehicle"

}

// test()


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
		this.world_origin = this.world_origin + vec2( 35 * delta_seconds, 0 )

		if ( smoke_puff )
		{
			smoke_puff.position = this.world_origin + smoke_offset
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

sprite <- Player()
sprite.world_origin = vec2( 150, 100 )
sprite.set_sprite( "sprites/player" )


// enemy <- Enemy()
// enemy.set_sprite( "sprites/enemy" )
// enemy.world_origin = vec2( 120, 100 )
