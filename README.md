# WonderlandGraphicsProject
Final Project for Computer Graphics module, December 2025, by Daniel Cunney.

---

Github: https://github.com/DanielCNY/WonderlandGraphicsProject.git

---

### **Instructions**

#### Controls:

    W --- Move Forward

    A --- Move Left

    S --- Move Right

    D --- Move Backward

    SPACE --- Move Up

    TAB --- Move Down

    MOUSE_CURSOR --- Look Around

#### Particle Effects: 

Due to rendering issues, particle effects block the skybox from being visible. That is why the following line is commented out:

<WonderlandGraphicsProject/scene/main.cpp> Line 472: 

    // snowSystem.render(vp);

Simply uncomment this line to observe animated particle effects mimicking snowfall perpetually around the player, with a clear background though.