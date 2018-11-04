#include "TCamera.h"

#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <cmath>
#include <algorithm>
#include <stdio.h>
#include "Mesh.h"

TCamera::TCamera()
    : m_PrevViewMatrix(1.f)
    , m_viewMatrix(1.f)
    , m_viewProjMatrix(1.f)
{
    // Default projection parameters
    m_aspect = 1.333333f;
    m_fovy = 45.0f;
    m_zNear = 1.0f;
    m_zFar = 10000.0f;
    setProjectionParams(m_fovy, 1.0f, m_zNear, m_zFar);

    // Default view parameters
    m_position = glm::vec3(0.0f);
    m_target = glm::vec3(0.0f, 0.0f, -1.0f);
    m_up = glm::vec3(0.f, 1.f, 0.f);

    m_forward = glm::vec3(0.f);

    // Camera control
    m_pitchAngle = 0.0f;
    m_yawAngle = 0.0f;

    m_moveCoef = 1.0f;
    m_rotationCoef = 0.01f;
    m_inertiaCoef = 0.85f;

    m_moveVelocity = glm::vec3(0.f);
    m_rotationVelocity = glm::vec2(0.f);

    m_bLimitPitchAngle = true;
    m_bInvertPitch = false;
    m_bInvertYaw = false;

    for (int i = 0; i < NUM_CAMERAKEYS; ++i)
        m_keydown[i] = false;

    m_bHasMoved = false;
    m_bHasLooked = false;

    m_cursorPos = glm::vec2(0.0f);
    m_cursorOldPos = glm::vec2(0.0f);
    m_cursorDelta = glm::vec2(0.0f);

    m_bEnableRotation = true;
    m_bEnableMove = true;

    update(0.f);
}

void TCamera::setProjectionParams(float fov, float aspect, float zNear, float zFar)
{
    m_fovy = fov;
    m_zNear = zNear;
    m_zFar = zFar;
    m_aspect = aspect;

    m_projectionMatrix = glm::perspective(glm::radians(m_fovy), m_aspect, m_zNear, m_zFar);
    m_viewProjMatrix = m_projectionMatrix * m_viewMatrix;
}

void TCamera::setViewParams(const glm::vec3 &pos, const glm::vec3 &target)
{
    m_position = pos;
    m_target = target;

    // .Compute the view direction 
    m_forward = m_target - m_position;
    m_forward = glm::normalize(m_forward);

    // .Compute the UP vector
    // treat the case where the direction vector is parallel to the Y Axis
    if ((fabs(m_forward.x) < FLT_EPSILON) && (fabs(m_forward.z) < FLT_EPSILON))
    {
        if (m_forward.y > 0.0f) {
            m_up = glm::vec3(0.0f, 0.0f, 1.0f);
        }
        else {
            m_up = glm::vec3(0.0f, 0.0f, -1.0f);
        }
    }
    else
    {
        m_up = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    m_Right = glm::cross(m_up, m_forward);
    m_Right = glm::normalize(m_Right);

    m_up = glm::cross(m_Right, m_forward);
    m_up = glm::normalize(m_up);

    // .Create the matrix
    m_viewMatrix = glm::lookAt(m_position, m_target, m_up);
    m_viewProjMatrix = m_projectionMatrix * m_viewMatrix;

    // .Retrieve the yaw & pitch angle  
    glm::vec3 zAxis = -m_forward;  // (it's also the third row of the viewMatrix)

    m_yawAngle = atan2f(zAxis.x, zAxis.z);

    float len = sqrtf(zAxis.x*zAxis.x + zAxis.z*zAxis.z);
    m_pitchAngle = -atan2f(zAxis.y, len);
}

void TCamera::keyboardHandler(CameraKeys key, bool bPressed)
{
    if (!m_bEnableMove) {
        return;
    }

    m_keydown[key] = bPressed;

    // Setting it to true will set a m_moveVelocity to the null vector
    //when the key will be released.
    m_bHasMoved = true;
}

void TCamera::motionHandler(int x, int y, bool bClicked)
{
    if (bClicked)
    {
        m_cursorPos = glm::vec2(x, y);
        m_cursorDelta = glm::vec2(0.0f);
    }
    else if (m_bEnableRotation)
    {
        m_cursorOldPos = m_cursorPos;
        m_cursorPos = glm::vec2(x, y);
        m_bHasLooked = true;
    }
}

bool TCamera::update(float deltaT)
{
    bool bUpdated = false;

    // .Update camera velocity  
    if (m_bHasMoved)
    {
        m_bHasMoved = false;

        glm::vec3 direction = glm::vec3(0.0f);
        if (m_keydown[MOVE_RIGHT])    direction.x += 1.0f;
        if (m_keydown[MOVE_LEFT])     direction.x -= 1.0f;
        if (m_keydown[MOVE_UP])       direction.y += 1.0f;
        if (m_keydown[MOVE_DOWN])     direction.y -= 1.0f;
        if (m_keydown[MOVE_BACKWARD]) direction.z += 1.0f;
        if (m_keydown[MOVE_FORWARD])  direction.z -= 1.0f;

        if ((direction.x != 0.0f) || (direction.y != 0.0f) || (direction.z != 0.0f)) {
            direction = glm::normalize(direction);
        }

        m_moveVelocity = m_moveCoef * direction;
    }

    float inertia = m_rotationVelocity.x*m_rotationVelocity.x +
        m_rotationVelocity.y*m_rotationVelocity.y;

    // .Update camera rotation
    if (m_bHasLooked || (inertia > FLT_EPSILON))
    {
        if (m_bHasLooked)
        {
            // interpolate to avoid jaggies
            m_cursorDelta = 0.70f*m_cursorDelta + 0.30f*(m_cursorPos - m_cursorOldPos);

            m_rotationVelocity = m_rotationCoef * m_cursorDelta;
        }
        else
        {
            // if the camera stop to move, add inertia.
            m_rotationVelocity *= m_inertiaCoef;
        }

        m_bHasLooked = false;

        float yawDelta = m_rotationVelocity.x;
        m_yawAngle += (m_bInvertYaw) ? yawDelta : -yawDelta;

        float pitchDelta = m_rotationVelocity.y;
        m_pitchAngle += (m_bInvertPitch) ? pitchDelta : -pitchDelta;

        if (m_bLimitPitchAngle)
        {
            m_pitchAngle = std::max(m_pitchAngle, float(-M_PI_2));
            m_pitchAngle = std::min(m_pitchAngle, float(+M_PI_2));
        }
    }

    // Compute the Rotate matrix
    // Ry(yaw) * Rx(pitch) * Rz(0.f)
    glm::mat3 cameraRotate = glm::mat3(glm::yawPitchRoll(m_yawAngle, m_pitchAngle, 0.0f));

    const glm::vec3 front(0.0f, 0.0f, -1.0f);
    m_forward = cameraRotate * front;
    m_forward = glm::normalize(m_forward);

    const glm::vec3 right(1.0f, 0.0f, 0.0f);
    m_Right  = cameraRotate * right;
    m_Right = glm::normalize(m_Right);

    // .Compute the new view parameters
    m_position += cameraRotate * m_moveVelocity;
    m_target = m_position + m_forward;
    m_up = glm::cross(m_Right, m_forward);

    m_projectionMatrix = glm::perspective(glm::radians(m_fovy), m_aspect, m_zNear, m_zFar);

    m_viewMatrix = glm::lookAt(m_position, m_target, m_up);
    m_viewProjMatrix = m_projectionMatrix * m_viewMatrix;

    if (m_PrevProjMatrix != m_projectionMatrix)
    {
        bUpdated = true;
        m_PrevProjMatrix = m_projectionMatrix;
    }

    if (m_PrevViewMatrix != m_viewMatrix)
    {
        bUpdated = true;
        m_PrevViewMatrix = m_viewMatrix;
    }

    updateFrustum();

    /**/

    //Having the camera model matrix can be helpful + it holds position / target & up in
    // its columns
    //camera = view^-1
    return bUpdated;
}

void TCamera::updateFrustum()
{
	// Just to visualise it http://www.panohelp.com/lensfov.html
	float nearHeight = 2 * glm::tan(m_fovy / 2) * m_zNear;
	float nearWidth = nearHeight * m_aspect;

	float farHeight = 2 * glm::tan(m_fovy / 2) * m_zFar;
	float farWidth = farHeight * m_aspect;

	glm::vec3 fc = m_position + m_forward * m_zFar;
	glm::vec3 nc = m_position + m_forward * m_zNear;

    m_Frustum.near = m_zNear;
    m_Frustum.far = m_zFar;
    m_Frustum.fovy = m_fovy;
    m_Frustum.aspect = m_aspect;

	m_Frustum.frustumCorners[4] = fc + (m_up * farHeight / 2.0f) - (m_Right * farWidth / 2.0f); // 
	m_Frustum.frustumCorners[5] = fc + (m_up * farHeight / 2.0f) + (m_Right * farWidth / 2.0f); // 
	m_Frustum.frustumCorners[7] = fc - (m_up * farHeight / 2.0f) - (m_Right * farWidth / 2.0f); // 
	m_Frustum.frustumCorners[6] = fc - (m_up * farHeight / 2.0f) + (m_Right * farWidth / 2.0f); // 

	m_Frustum.frustumCorners[0] = nc + (m_up * nearHeight / 2.0f) - (m_Right * nearWidth / 2.0f); // 
	m_Frustum.frustumCorners[1] = nc + (m_up * nearHeight / 2.0f) + (m_Right * nearWidth / 2.0f); // 
	m_Frustum.frustumCorners[3] = nc - (m_up * nearHeight / 2.0f) - (m_Right * nearWidth / 2.0f); // 
	m_Frustum.frustumCorners[2] = nc - (m_up * nearHeight / 2.0f) + (m_Right * nearWidth / 2.0f); // 


	//                         0--------1
	//                        /|       /|
	//     Y ^               / |      / |
	//     | _              4--------5  |
	//     | /' -Z          |  |     |  |
	//     |/               |  2-----|--3
	//     + ---> X         | /      | /
	//                      |/       |/
	//                      6--------7
}

