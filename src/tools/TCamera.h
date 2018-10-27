#ifndef TCAMERA_HPP
#define TCAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <cmath>
#include <array>

enum CameraKeys
{
    MOVE_FORWARD = 0,
    MOVE_BACKWARD,
    MOVE_LEFT,
    MOVE_RIGHT,
    MOVE_UP,
    MOVE_DOWN,

    NUM_CAMERAKEYS
};

struct Frustum
{
    float near, far, fovy, aspect;
    std::array<glm::vec3, 8> frustumCorners;
};

class TCamera
{
protected:

    glm::mat4 m_PrevProjMatrix;
    glm::mat4 m_PrevViewMatrix;
    glm::mat4 m_projectionMatrix;
    glm::mat4 m_viewMatrix;
    glm::mat4 m_viewProjMatrix;

    // Projection parameters
    float m_fovy;
    float m_aspect;
    float m_zNear;
    float m_zFar;

    Frustum m_Frustum;

    // Look At parameters
    glm::vec3 m_position;               // camera (eye) position
    glm::vec3 m_target;                 // camera target
    glm::vec3 m_up;                     // camera UP vector, compute automatically
    glm::vec3 m_forward;                // direction of the camera
    glm::vec3 m_Right;                  // right

    // Camera control    
    float m_pitchAngle;                 // x-axis rotation angle (in radians)
    float m_yawAngle;                   // y-axis rotation angle (in radians)
    float m_moveCoef;
    float m_rotationCoef;
    float m_inertiaCoef;
    glm::vec3 m_moveVelocity;
    glm::vec2 m_rotationVelocity;
    bool m_bLimitPitchAngle;            // if true, pitch is limited in the range [-pi/2, pi/2]
    bool m_bInvertPitch;                // if true, inverts the pitch angle
    bool m_bInvertYaw;                  // if true, inverts the yaw angle    

    // Events control
    bool m_keydown[NUM_CAMERAKEYS];
    bool m_bHasMoved;                   // True when camera has moved
    bool m_bHasLooked;                  // True when camera has rotated
    glm::vec2 m_cursorPos;
    glm::vec2 m_cursorOldPos;
    glm::vec2 m_cursorDelta;

    bool m_bEnableRotation;
    bool m_bEnableMove;

public:

    TCamera();

    // .Update the camera attributes with user input
    bool update(float deltaT = 1.0f);
    void updateFrustum();

    // .EVENT HANDLERS
    void keyboardHandler(CameraKeys key, bool bPressed);
    void motionHandler(int x, int y, bool bClicked);

    // .SETTERS
    void setProjectionParams(float fov, float aspect, float zNear, float zFar);
    void setViewParams(const glm::vec3 &pos, const glm::vec3 &target);

    void setMoveCoefficient(float coef) { m_moveCoef = coef; }
    void setRotationCoefficient(float coef) { m_rotationCoef = coef; }
    void setInertiaCoefficient(float coef) { m_inertiaCoef = coef; }

    void doLimitXAxis(bool state) { m_bLimitPitchAngle = state; }
    void doInvertXAxis(bool state) { m_bInvertPitch = state; }
    void doInvertYAxis(bool state) { m_bInvertYaw = state; }

    void doEnableMove(bool state) { m_bEnableMove = state; }
    void doEnableRotation(bool state) { m_bEnableRotation = state; }

    // .GETTERS
    const glm::mat4& getPrevProjectionMatrix() const { return m_PrevProjMatrix; }
    const glm::mat4& getProjectionMatrix() const { return m_projectionMatrix; }
    const glm::mat4& getPrevViewMatrix() const { return m_PrevViewMatrix; }
    const glm::mat4& getViewMatrix() const { return m_viewMatrix; }
    const glm::mat4& getViewProjMatrix() const { return m_viewProjMatrix; }

    // Or use the row of the view matrix
    const glm::vec3& getPosition() const { return m_position; }
    const glm::vec3& getTarget() const { return m_target; }
    const glm::vec3& getDirection() const { return m_forward; }

    void setFov(float fov) { m_fovy = fov; }

    float getMoveCoefficient() const { return m_moveCoef; }
    float getRotationCoefficient() const { return m_rotationCoef; }
    float getInertiaCoefficient() const { return m_inertiaCoef; }
    float getFar() const { return m_zFar; }
    float getNear() const { return m_zNear; }
    float getFov() const { return m_fovy; }
    float getAspect() const { return m_aspect; }
    const Frustum& getFrustum() const { return m_Frustum; }

    bool isXAxisLimited() const { return m_bLimitPitchAngle; }
    bool isXAxisInverted() const { return m_bInvertPitch; }
    bool isYAxisInverted() const { return m_bInvertYaw; }
};

#endif // TCAMERA_HPP