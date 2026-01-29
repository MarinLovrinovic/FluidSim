Ideas:
- Add internal pressure
- Connect particle to neighbors of neighbors
- Add internal particles
- Remove duplicate spring


volume calculation
    general, but seems complex

filled with particles
    seems hard to get those particles and connect them to existing ones
    options:
        make (multiple) scaled down copies of the whole thing and connect the layers together.
            may be a bit unstable if each particle is connected to just one from the other layer.
                perhaps connect it to the one from the other layer AND its neighbors on that same layer.
                    sounds like indexing hell. Would probably need some kind of universal index for each original
                    particle and its copies.
        make a grid of particles inside the 3D model.
            how do I figure out if particle is inside 3D model?
                Raycasting. Odd number of hits means inside, even number of hits means outside.
            how do I connect it to surface particles?
                The first time in one row of searching that a particle is found, connect it to 3 nearest surface particles
