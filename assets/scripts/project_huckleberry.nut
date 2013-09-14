print( "Hello from squirrel!\n" )

function test()
{
	print( "setting up game\n" )


	local e = Entity()

	e.name = "Vehicle"

}

// test()


class RotatingModel extends ModelEntity
{
	function tick()
	{
		local delta_angle = 0.5

		// rotate around the Y-axis
		this.transform = rotate( this.transform, delta_angle, vec3(0, 1, 0) )
	}
}

ground <- ModelEntity()
ground.name = "ground"
ground.set_model( "models/room2")

vehicle <- RotatingModel()
vehicle.name = "Vehicle"
vehicle.transform = translate( vehicle.transform, vec3(0, 0.3432, 0.0) )
vehicle.set_model( "models/vehicle" )

