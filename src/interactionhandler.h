#ifndef INTERACTIONHANDLER_H
#define INTERACTIONHANDLER_H

// open space includes
#include "object.h"
#include "camera.h"
#include "externalcontrol/externalcontrol.h"
#include "scenegraph/scenegraphnode.h"

// std includes
#include <vector>
#include <thread>
#include <mutex>
#include <memory>

// hack until we have a file/path manager
//#ifdef __WIN32__
//#ifdef NDEBUG 
//#define RELATIVE_PATH ""
//#else
//#define RELATIVE_PATH "../../../"
//#endif
//#else
//#define RELATIVE_PATH "../"
//#endif

namespace openspace {

class InteractionHandler: public Object {
public:
	virtual ~InteractionHandler();

	static void init();
	static void deinit();
    static InteractionHandler& ref();
	static bool isInitialized();

	void enable();
	void disable();
	const bool isEnabled() const;

	void connectDevices();
	void addExternalControl(ExternalControl* controller);

	void setCamera(Camera *camera = nullptr);
	Camera * getCamera() const;
	const psc getOrigin() const;
	void lockControls();
	void unlockControls();

	void setFocusNode(SceneGraphNode *node);
	
	void orbit(const glm::quat &rotation);
	void rotate(const glm::quat &rotation);
	void distance(const pss &distance);

	void lookAt(const glm::quat &rotation);
	void setRotation(const glm::quat &rotation);

	void update(const double dt);

	double getDt();
	
private:
	static InteractionHandler* this_;
    InteractionHandler(void);
    InteractionHandler(const InteractionHandler& src);
    InteractionHandler& operator=(const InteractionHandler& rhs);

	Camera *camera_;
	bool enabled_;
	SceneGraphNode *node_;
	
	double dt_;

	// used for calling when updating and deallocation
	std::vector<ExternalControl*> controllers_;

	// for locking and unlocking
	std::mutex cameraGuard_;
	
};

} // namespace openspace

#endif