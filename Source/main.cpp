#include <Urho3D/Engine/Application.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Terrain.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Model.h>

// This is probably always OK.
using namespace Urho3D;


class AwesomeGameApplication : public Application
{
    // This macro defines some methods that every `Urho3D::Object` descendant should have.
    URHO3D_OBJECT(AwesomeGameApplication, Application);
	
	SharedPtr<Scene> scene_;
	SharedPtr<Node> cameraNode_;
	
	float pitch_=0;
	float yaw_=0;
	
	
public:
    // Likewise every `Urho3D::Object` descendant should implement constructor with single `Context*` parameter.
    AwesomeGameApplication(Context* context)
        : Application(context)
    {
    }

    void Setup() override
    {
        // Engine is not initialized yet. Set up all the parameters now.
        engineParameters_[EP_FULL_SCREEN] = false;
        engineParameters_[EP_WINDOW_HEIGHT] = 600;
        engineParameters_[EP_WINDOW_WIDTH] = 800;
        // Resource prefix path is a list of semicolon-separated paths which will be checked for containing resource directories. They are relative to application executable file.
        engineParameters_[EP_RESOURCE_PREFIX_PATHS] = ".;..";
    }

    void Start() override
    {
		auto* renderer = GetSubsystem<Renderer>();
		auto* cache = GetSubsystem<ResourceCache>();
        // At this point engine is initialized, but first frame was not rendered yet. Further setup should be done here. To make sample a little bit user friendly show mouse cursor here.
        GetSubsystem<Input>()->SetMouseVisible(true);
		
		scene_ = new Scene(context_);

		// Create octree, use default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
		scene_->CreateComponent<Octree>();

		// Create a Zone component for ambient lighting & fog control
		Node* zoneNode = scene_->CreateChild("Zone");
		auto* zone = zoneNode->CreateComponent<Zone>();
		zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
		zone->SetAmbientColor(Color(0.15f, 0.15f, 0.15f));
		zone->SetFogColor(Color(1.0f, 1.0f, 1.0f));
		zone->SetFogStart(500.0f);
		zone->SetFogEnd(750.0f);

		// Create a directional light to the world. Enable cascaded shadows on it
		Node* lightNode = scene_->CreateChild("DirectionalLight");
		lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f));
		auto* light = lightNode->CreateComponent<Light>();
		light->SetLightType(LIGHT_DIRECTIONAL);
		light->SetCastShadows(true);
		light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
		light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));
		light->SetSpecularIntensity(0.5f);
		// Apply slightly overbright lighting to match the skybox
		light->SetColor(Color(1.2f, 1.2f, 1.2f));

		// Create skybox. The Skybox component is used like StaticModel, but it will be always located at the camera, giving the
		// illusion of the box planes being far away. Use just the ordinary Box model and a suitable material, whose shader will
		// generate the necessary 3D texture coordinates for cube mapping
		Node* skyNode = scene_->CreateChild("Sky");
		skyNode->SetScale(500.0f); // The scale actually does not matter
		auto* skybox = skyNode->CreateComponent<Skybox>();
		skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
		skybox->SetMaterial(cache->GetResource<Material>("Materials/Skybox.xml"));

		// Create heightmap terrain
		Node* terrainNode = scene_->CreateChild("Terrain");
		terrainNode->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
		auto* terrain = terrainNode->CreateComponent<Terrain>();
		terrain->SetPatchSize(64);
		terrain->SetSpacing(Vector3(2.0f, 0.5f, 2.0f)); // Spacing between vertices and vertical resolution of the height map
		terrain->SetSmoothing(true);
		terrain->SetHeightMap(cache->GetResource<Image>("Textures/HeightMap.png"));
		terrain->SetMaterial(cache->GetResource<Material>("Materials/TerrainArrays.xml"));
		// The terrain consists of large triangles, which fits well for occlusion rendering, as a hill can occlude all
		// terrain patches and other objects behind it
		terrain->SetOccluder(true);
		cameraNode_ = new Node(context_);
		auto* camera = cameraNode_->CreateComponent<Camera>();
		camera->SetFarClip(750.0f);

		// Set an initial position for the camera scene node above the ground
		cameraNode_->SetPosition(Vector3(0.0f, 7.0f, -20.0f));
		SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
		renderer->SetViewport(0, viewport);
		
		SubscribeToEvent(StringHash("Update"), URHO3D_HANDLER(AwesomeGameApplication, HandleUpdate));
    }

    void Stop() override
    {
        // This step is executed when application is closing. No more frames will be rendered after this method is invoked.
    }
	
	void MoveCamera(float timeStep)
	{
		auto* input = GetSubsystem<Input>();

		// Movement speed as world units per second
		const float MOVE_SPEED = 20.0f;
		// Mouse sensitivity as degrees per pixel
		const float MOUSE_SENSITIVITY = 0.1f;

		// Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
		IntVector2 mouseMove = input->GetMouseMove();
		yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
		pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
		pitch_ = Clamp(pitch_, -90.0f, 90.0f);

		// Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
		cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
		
		// Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
		if (input->GetKeyDown(KEY_W))
			cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
		if (input->GetKeyDown(KEY_S))
			cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
		if (input->GetKeyDown(KEY_A))
			cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
		if (input->GetKeyDown(KEY_D))
			cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);
	}

	void HandleUpdate(StringHash eventType, VariantMap& eventData)
	{
		//using namespace Update;

		// Take the frame time step, which is stored as a float
		float timeStep = eventData[StringHash("TimeStep")].GetFloat();

		// Move the camera, scale movement with time step
		MoveCamera(timeStep);
	}
};

// A helper macro which defines main function. Forgetting it will result in linker errors complaining about missing `_main` or `_WinMain@16`.
URHO3D_DEFINE_APPLICATION_MAIN(AwesomeGameApplication);