/*
* Library osgLeap
* Copyright (C) 2013 Johannes Kroeger/vtxtech. All rights reserved.
*
* This file is licensed under the GNU Lesser General Public License 3 (LGPLv3),
* but distributed WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
*/

#include <osgLeap/HandState>

//-- OSG: osgDB --//
#include <osgDB/ReadFile>

namespace osgLeap {

	typedef std::vector<osg::ref_ptr<osg::Image> > ImageVector;
	static ImageVector sHandsTextures;

	// UpdateCallback "auto-updates" the osgLeap::HandState Geode from within
    // the update traversal of the osgViewer
    class UpdateCallback: public osg::NodeCallback
    {
    public:
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            HandState* hs = dynamic_cast<HandState*>(node);
            if (hs) {
                hs->update();
            }
            traverse(node, nv);
        }
    };

    void HandState::createHandQuad(WhichHand hand)
    {
        // Just create a textured quad here...
        // LEFT_HAND and RIGHT_HAND differs in
        // - vertex coordinates,
        // - texture coordinates and
        // - the texture itself.

        osg::ref_ptr<osg::Geometry> geom = new osg::Geometry();
        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array();
        geom->setVertexArray(va);
        osg::ref_ptr<osg::Vec3Array> na = new osg::Vec3Array();
        na->push_back(osg::Vec3(0.0f, 0.0f, 1.0f));
#ifndef PRE_OSG_320_ARRAYBINDINGS
		geom->setNormalArray(na, osg::Array::BIND_PER_PRIMITIVE_SET);
#else
		geom->setNormalArray(na);
		geom->setNormalBinding(osg::Geometry::BIND_PER_PRIMITIVE_SET);
#endif
        osg::Vec4Array* colors = new osg::Vec4Array();
        colors->push_back(osg::Vec4(1.0f,1.0f,1.0f,1.0f));
#ifndef PRE_OSG_320_ARRAYBINDINGS
        geom->setColorArray(colors, osg::Array::BIND_OVERALL);
#else
		geom->setColorArray(colors);
		geom->setColorBinding(osg::Geometry::BIND_OVERALL);
#endif
        geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,4));

        osg::ref_ptr<osg::Vec2Array> texCoords = new osg::Vec2Array();

        if (hand == LEFT_HAND) {
            va->push_back(osg::Vec3(0.0f, 0.0f, 0.0f));
            va->push_back(osg::Vec3(128.0f, 0.0f, 0.0f));
            va->push_back(osg::Vec3(128.0f, 160.0f, 0.0f));
            va->push_back(osg::Vec3(0.0f, 160.0f, 0.0f));

            texCoords->push_back(osg::Vec2(1,0));
            texCoords->push_back(osg::Vec2(0,0));
            texCoords->push_back(osg::Vec2(0,1));
            texCoords->push_back(osg::Vec2(1,1));

            geom->getOrCreateStateSet()->setTextureAttributeAndModes(0, lhTex_, osg::StateAttribute::ON);
        } else {
            va->push_back(osg::Vec3(128.0f, 0.0f, 0.0f));
            va->push_back(osg::Vec3(256.0f, 0.0f, 0.0f));
            va->push_back(osg::Vec3(256.0f, 160.0f, 0.0f));
            va->push_back(osg::Vec3(128.0f, 160.0f, 0.0f));

            texCoords->push_back(osg::Vec2(0,0));
            texCoords->push_back(osg::Vec2(1,0));
            texCoords->push_back(osg::Vec2(1,1));
            texCoords->push_back(osg::Vec2(0,1));

            geom->getOrCreateStateSet()->setTextureAttributeAndModes(0, rhTex_, osg::StateAttribute::ON);
        }

        geom->setTexCoordArray(0, texCoords);
        geom->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
        addDrawable(geom);
    }

    HandState::HandState(): osg::Geode(), Leap::Listener(),
        frame_(Leap::Frame()),
        lhTex_(new osg::Texture2D()),
        rhTex_(new osg::Texture2D())
    {
        // Initialize UpdateCallback to update myself during updateTraversal
        addUpdateCallback(new UpdateCallback());

        controller_.addListener(*this);

        std::vector<std::string> filenames;
        filenames.push_back("nohand.png");
        filenames.push_back("hand0.png");
        filenames.push_back("hand1.png");
        filenames.push_back("hand2.png");
        filenames.push_back("hand3.png");
        filenames.push_back("hand4.png");
        filenames.push_back("hand5.png");

		if (sHandsTextures.size() == 0) { 
			// Load textures for hands visualization.
			// Note that all hands images must be either in the same
			// directory as the executable, or in the current working
			// directory or in a path that is defined in the OSG_FILE_PATH
			// system environment variable
			for (int i = 0; i < filenames.size(); ++i) {
        		osg::ref_ptr<osg::Image> img = osgDB::readImageFile(filenames.at(i));
        		if (img.valid()) {
            		sHandsTextures.push_back(img);
        		} else {
        			OSG_FATAL<<"osgLeap::HandState constructor: Failed to read hands image '"<<filenames.at(i)<<"'."<<std::endl;
        		}
			}

			// Prescale images to square resolution so we avoid doing that
			// during update
			sHandsTextures.at(0)->scaleImage(1024, 1024, 1);
			sHandsTextures.at(1)->scaleImage(1024, 1024, 1);
			sHandsTextures.at(2)->scaleImage(1024, 1024, 1);
			sHandsTextures.at(3)->scaleImage(1024, 1024, 1);
			sHandsTextures.at(4)->scaleImage(1024, 1024, 1);
			sHandsTextures.at(5)->scaleImage(1024, 1024, 1);
			sHandsTextures.at(6)->scaleImage(1024, 1024, 1);
		}

        if (sHandsTextures.size() < filenames.size()) {
        	OSG_FATAL<<"osgLeap::HandState constructor: Failed to read hands images. Got: "<<sHandsTextures.size()<<", expected: "<<filenames.size()<<std::endl;
        	return;
        }

        // Set DataVariance to DYNAMIC to avoid the texture changes being
        // optimized away.
        lhTex_->setDataVariance(osg::Object::DYNAMIC);
        lhTex_->setImage(sHandsTextures.at(0));
        rhTex_->setDataVariance(osg::Object::DYNAMIC);
        rhTex_->setImage(sHandsTextures.at(0));

        // Now finally, create the QUAD geometry to put our texture onto
        // Note that this method should be called once per hand, only.
        createHandQuad(LEFT_HAND);
        createHandQuad(RIGHT_HAND);
    }

    HandState::~HandState()
    {
        controller_.removeListener(*this);
    }

    HandState::HandState(const HandState& hs,
        const osg::CopyOp& copyOp): osg::Geode(*this), Leap::Listener(*this),
        frame_(Leap::Frame())
    {
        // ToDo/j.kroeger: Copy texture2d member variables (lhTex_, rhTex_) correctly...
    }

    void HandState::onFrame(const Leap::Controller& controller)
    {
        // Get the most recent frame and store it for later use in update(...)
        frame_ = controller.frame();
    }

    void HandState::update()
    {
        // Grab the frame to work on ...
        Leap::Frame frame = frame_;

        // Setup "no-hand" image as default
        osg::Image* lh = sHandsTextures.at(0);
        osg::Image* rh = sHandsTextures.at(0);

        // Continue if there it at least one hand, only.
        if (frame.hands().count() > 0) {
            // Using leftmost and rightmost hands
            Leap::Hand left = frame.hands().leftmost();
            Leap::Hand right = frame.hands().rightmost();
            // Count the fingers we have detected...
#ifdef LEAPSDK_1X_COMPATIBILITY
            int r_fingers = right.fingers().count()+1;
            int l_fingers = left.fingers().count()+1;
#else
            int r_fingers = right.fingers().extended().count()+1;
			int l_fingers = left.fingers().extended().count()+1;
#endif
            // Avoid crash if textures were not loaded
            // or if we have more than 5 fingers per hand ;-)
            if (r_fingers > sHandsTextures.size()) {
                OSG_WARN<<"WARN: Not enough images ("<<sHandsTextures.size()<<") for right hand finger count ("<<r_fingers-1<<"), aborting HandState::update."<<std::endl;
                return;
            }
            if (l_fingers > sHandsTextures.size()) {
                OSG_WARN<<"WARN: Not enough images ("<<sHandsTextures.size()<<") for left hand finger count ("<<l_fingers-1<<"), aborting HandState::update."<<std::endl;
                return;
            }
            // Compare hands IDs to determine if leftmost hand and rightmost
            // hand are the same
            if (left.id() == right.id()) {
                // Assume right hand if we have one hand, only.
                // (As we cannot distinguish between the actual right and left
                // hand. We operate on "leftmost" and "rightmost" hands only.)
                rh = sHandsTextures.at(r_fingers);
            } else {
                rh = sHandsTextures.at(r_fingers);
                lh = sHandsTextures.at(l_fingers);
            }
        }

        lhTex_->setImage(lh);
        rhTex_->setImage(rh);
    }

}
