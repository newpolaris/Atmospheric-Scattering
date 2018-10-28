#include "ShadowTechnique.h"
#include "BufferManager.h"

using namespace Graphics;

void Graphics::CalcOrthoProjections(
    const SceneSettings& settings,
    const TCamera& camera,
    const glm::vec3& direction,
    std::vector<float>& m_ClipspaceCascadeEnd,
    std::vector<glm::mat4>& lightSpace)
{
    std::vector<float> cascadeEnd(Graphics::g_NumShadowCascade+1);

    if (settings.bClipSplitLogUniform)
    {
        auto frustum = camera.getFrustum();

        // Between 0 and 1, change these to check the results
        float minDistance = 0.0f;
        float maxDistance = 1.0f;

        auto nearClip = frustum.near;
        auto farClip = frustum.far;
        auto clipRange = farClip - nearClip;

        auto minZ = nearClip + minDistance * clipRange;
        auto maxZ = nearClip + maxDistance * clipRange;

        auto range = maxZ - minZ;
        auto ratio = maxZ / minZ;

        // to get the most optimal split distances,
        // it works by using a logarithmic and uniform split scheme 
        // detail by Nvidia here: http://http.developer.nvidia.com/GPUGems3/gpugems3_ch10.html 
        for (uint32_t i = 0; i <= Graphics::g_NumShadowCascade; i++)
        {
            auto p = float(i) / Graphics::g_NumShadowCascade;
            auto log = minZ * glm::pow(ratio, p);
            auto uniform = minZ + range * p;
            auto d = settings.lambda * (log - uniform) + uniform;
            cascadeEnd[i] = -d;
        }
    }
    else
    {
        cascadeEnd[0] = -camera.getNear();
        cascadeEnd[1] = -settings.Slice1;
        cascadeEnd[2] = -settings.Slice2;
        cascadeEnd[2] = -settings.Slice3;
        cascadeEnd[3] = -camera.getFar();
    }

    glm::mat4 viewInv = glm::inverse(camera.getViewMatrix());

    float aspect = (float)Graphics::g_NativeWidth/Graphics::g_NativeHeight;
    float halfFovY = glm::radians(settings.fov / 2.f);
    float tanHalfVerticalFOV = glm::tan(halfFovY);
    float tanHalfHorizontalFOV = tanHalfVerticalFOV*aspect;

    lightSpace.resize(Graphics::g_NumShadowCascade);
    for (uint32_t i = 0; i < Graphics::g_NumShadowCascade; i++)
    {
        float xn = cascadeEnd[i] * tanHalfHorizontalFOV;
        float yn = cascadeEnd[i] * tanHalfVerticalFOV;
        float xf = cascadeEnd[i+1] * tanHalfHorizontalFOV;
        float yf = cascadeEnd[i+1] * tanHalfVerticalFOV;

        auto frustumCorners = {
            // near face
            glm::vec4( xn,  yn, cascadeEnd[i], 1.f),
            glm::vec4(-xn,  yn, cascadeEnd[i], 1.f),
            glm::vec4( xn, -yn, cascadeEnd[i], 1.f),
            glm::vec4(-xn, -yn, cascadeEnd[i], 1.f),

            // far face
            glm::vec4( xf,  yf, cascadeEnd[i+1], 1.f),
            glm::vec4(-xf,  yf, cascadeEnd[i+1], 1.f),
            glm::vec4( xf, -yf, cascadeEnd[i+1], 1.f),
            glm::vec4(-xf, -yf, cascadeEnd[i+1], 1.f),
        };

        if (settings.bBoundSphere)
        {
            std::vector<glm::vec3> frustumCornersWS;

            // Transform the frustum coordinate from view to world space
            for (auto it : frustumCorners)
                frustumCornersWS.push_back(glm::vec3(viewInv * it));

            glm::vec3 center(0.f);
            for (auto it : frustumCornersWS)
                center += it;
            center /= 8.f;

            auto radius = 0.f;
            for (auto it : frustumCornersWS)
                radius = glm::max(radius, glm::length(it - center));
            radius = glm::ceil(radius * 16.f) / 16.f;

            auto maxExtents = glm::vec3(radius);
            auto minExtents = glm::vec3(-radius);

            auto lightDirection = glm::normalize(direction);

            // Position the viewmatrix looking down the center of the frustum with an arbitrary lighht direction
            glm::vec3 position = center - lightDirection * -minExtents.z;
            auto lightview = glm::lookAt(position, center, glm::vec3(0.0f, 1.0f, 0.0f));

            glm::vec3 cascadeExtents = maxExtents - minExtents;
            auto lightproject = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.f, cascadeExtents.z);

            if (settings.bReduceShimmer)
            {
                auto shadowmapSize =  float(Graphics::g_ShadowMapSize);
                // The rounding matrix that ensures that shadow edges do not shimmer
                auto shadowspace = lightproject * lightview;
                auto shadowOrigin = glm::vec4(0.f, 0.f, 0.f, 1.f);
                shadowOrigin = shadowspace * shadowOrigin;
                shadowOrigin = shadowOrigin * shadowmapSize / 2.f;

                auto roundedOrigin = glm::round(shadowOrigin);
                auto roundOffset = roundedOrigin - shadowOrigin;
                roundOffset = roundOffset * 2.f / shadowmapSize;
                lightproject[3] += glm::vec4(roundOffset.x, roundOffset.y, 0.f, 0.f);
            }
            lightSpace[i] = lightproject * lightview;

        #if _DEBUG
            for (auto it : frustumCornersWS)
            {
                auto pts = lightSpace[i] * glm::vec4(it, 1.f);
                auto uvs = pts * 0.5f + 0.5f;
                assert(uvs.x <= 1.f && uvs.x >= 0.f);
                assert(uvs.y <= 1.f && uvs.y >= 0.f);
            }
        #endif
        }
        else
        {
            // From ogldev on tutorial49
            // "Since we are dealing with a directional light that has no origin 
            //  we just need to rotate the world so that the light direction becomes 
            //  aligned with the positive Z axis. The origin of light can simply be
            //  the origin of the light space coordinate system (which means we don't 
            //  need any translation)"
            auto lightview = glm::lookAt(glm::vec3(0.f), direction, glm::vec3(0.0, 1.0, 0.0));

            glm::vec3 minPoint = glm::vec3(std::numeric_limits<float>::max());
            glm::vec3 maxPoint = glm::vec3(-std::numeric_limits<float>::max());

            for (auto it : frustumCorners)
            {
                // Transform the frustum coordinate from view to world space
                glm::vec4 positionWS = viewInv * it;
                // Transform the frustum coordinate from world to light space
                glm::vec3 positionLS = glm::vec3(lightview * positionWS);

                minPoint = glm::min(minPoint, positionLS);
                maxPoint = glm::max(maxPoint, positionLS);
            }
            // glm::orth espect camera n, f which is > 0, so revert it
            auto lightproject = glm::ortho(minPoint.x, maxPoint.x, minPoint.y, maxPoint.y, -maxPoint.z, -minPoint.z);

            lightSpace[i] = lightproject * lightview;
        }
    }

    const auto project = camera.getProjectionMatrix();
    m_ClipspaceCascadeEnd.resize(Graphics::g_NumShadowCascade);
    for (uint32_t i = 0; i < Graphics::g_NumShadowCascade; i++)
    {
        glm::vec4 pointVS(0.f, 0.f, cascadeEnd[i + 1], 1.f);
        glm::vec4 pointCS = project * pointVS;
        // To compare with one that not perspective divide
        m_ClipspaceCascadeEnd[i] = pointCS.z;
    }
}
