#pragma once

void SendInputAimbot(int width, int height, float screenX, float screenY)
{

	// Calculate how far the target point is from the screen center
	float deltaX = screenX - width / 2.0f;   // Horizontal offset
	float deltaY = screenY - height / 2.0f;   // Vertical offset

	float smoothFactor = 5.0f; // Higher value = slower, smoother movement

	if (std::abs(deltaX) > 0.5f || std::abs(deltaY) > 0.5f)
	{

		// Divide the delta by the smooth factor
		// If delta is 100, and smooth is 5, we only move 20 pixels this frame.
		float smoothX = deltaX / smoothFactor;
		float smoothY = deltaY / smoothFactor;

		INPUT input = {};
		input.type = INPUT_MOUSE;
		input.mi.dwFlags = MOUSEEVENTF_MOVE;
		input.mi.dx = smoothX;
		input.mi.dy = smoothY;
		SendInput(1, &input, sizeof(INPUT));

	}


}