#pragma once

#include "utils.cuh"

class setupMenu
{
public:

	setupMenu() = default;
	~setupMenu() = default;

	void initialize();

	void update();

	// Returns true if grid is confirmed
	bool setupPanel();

private:

	bool gridCreationPanel();
	
	void drawGrids();

	int getMinimumSize(int i);


	// Own params, not connected to other systems
	simParams m_params;


};