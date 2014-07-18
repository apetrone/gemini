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


// ground <- ModelEntity()
// ground.name = "ground"
// ground.set_model( "models/room2")

// vehicle <- RotatingModel()
// vehicle.name = "Vehicle"
// vehicle.transform = translate( vehicle.transform, vec3(0, 0.3432, 0.0) )
// vehicle.set_model( "models/vehicle" )

class TestRules extends GameRules
{
	function startup()
	{
		print("Starting up TestRules...\n")
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

gamerules <- TestRules()