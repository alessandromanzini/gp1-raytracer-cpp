#pragma once
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Maths.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle) :
			origin{ _origin },
			fovAngle{ _fovAngle }
		{
			FovChanged( );
		}

		const float MOVEMENT_SPEED{ 3.f };
		const float CAMERA_ROTATION_SPEED{ 0.001f };

		Vector3 origin{};
		float fovAngle{ 90.f };
		float fovCoefficient{};

		Vector3 forward{ Vector3::UnitZ };
		Vector3 up{ Vector3::UnitY };
		Vector3 right{ Vector3::UnitX };

		float totalPitch{ 0.f };
		float totalYaw{ 0.f };

		Matrix cameraToWorld{};

		Matrix CalculateCameraToWorld()
		{
			cameraToWorld = Matrix {
				{ right.x, right.y, right.z, 0.f },
				{ up.x, up.y, up.z, 0.f },
				{ forward.x, forward.y, forward.z, 0.f },
				{ origin.x, origin.y, origin.z, 1.f }
			};
			return cameraToWorld;
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);
			ProcessKeyboardInput( pKeyboardState, deltaTime );

			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);
			const bool leftButtonPressed = mouseState & SDL_BUTTON( SDL_BUTTON_LMASK );
			if ( leftButtonPressed && (mouseX || mouseY) )
			{
				// Only process mouse input if the mouse position has changed.
				// Since the rotation matrix calculation is expensive, we only want to do it when necessary.
				ProcessMouseInput( mouseX, mouseY, deltaTime );
			}
		}

		void ApplyCameraRotations( )
		{
			Matrix rotationMatrix{ Matrix::CreateRotationX( totalPitch ) * Matrix::CreateRotationY( totalYaw ) };

			// Define forward as the rotation of the z-axis, normalized
			forward = rotationMatrix.TransformVector( Vector3::UnitZ );
			forward.Normalize( );

			// Define right and up from the forward vector and normalize them
			right = Vector3::Cross( Vector3::UnitY, forward );
			right.Normalize( );
			up = Vector3::Cross( forward, right );
			up.Normalized( );
		}

		inline void FovChanged( )
		{
			fovCoefficient = tanf( fovAngle * 0.5f * PI / 180.f ); // tan(fov/2)
		}

	private:
		inline void ProcessKeyboardInput( const uint8_t* pKeyboardState, float deltaTime )
		{
			const float speed{ deltaTime * MOVEMENT_SPEED };

			if ( pKeyboardState[SDL_SCANCODE_W] )
			{
				origin += forward * speed;
			}
			if ( pKeyboardState[SDL_SCANCODE_A] )
			{
				origin += -right * speed;
			}
			if ( pKeyboardState[SDL_SCANCODE_S] )
			{
				origin += -forward * speed;
			}
			if ( pKeyboardState[SDL_SCANCODE_D] )
			{
				origin += right * speed;
			}
		}

		inline void ProcessMouseInput( int mouseX, int mouseY, float deltaTime )
		{
			const float speed{ deltaTime * CAMERA_ROTATION_SPEED };

			totalYaw += mouseX * speed;
			totalPitch += mouseY * speed;
			NormalizeRotationAngle( totalYaw );
			NormalizeRotationAngle( totalPitch );

			ApplyCameraRotations( );
		}

		static inline void NormalizeRotationAngle( float& angle )
		{
			// Normalize the angle to be between -2PI and 2PI
			if ( angle == 0.f )
			{
				return;
			}

			const float direction{ abs( angle ) / angle };
			while ( angle * direction > 2 * PI )
			{
				angle -= 2 * PI * direction;
			}
		}

	};
}
