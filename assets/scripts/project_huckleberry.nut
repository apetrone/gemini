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



class Player extends SpriteEntity
{
	smoke_puff = null


	constructor()
	{
		base.constructor()

		// smoke_puff = SpriteEntity()
		// smoke_puff.set_sprite( "sprites/enemy" )
		// smoke_puff.world_origin = vec2(0, 0)

		//smoke_puff = EmitterEntity()
		//smoke_puff.set_emitter( "sprites/exhaust" )
	}
}

sprite <- Player()
sprite.set_sprite( "sprites/player" )
sprite.world_origin = vec2( 100, 100 )

puff <- EmitterEntity()
puff.set_emitter( "sprites/exhaust" )
