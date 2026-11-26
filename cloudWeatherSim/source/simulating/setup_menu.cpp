#include "simulating/setup_menu.hpp"

#include "cloud_hub.hpp"
#include "camera.hpp"

#include "rendering/render_hub.hpp"
#include "rendering/shapes_gl.hpp"
#include "rendering/colors.hpp"

#include <imgui/imgui.h>
#include <glm/glm.hpp>

void setupMenu::initialize()
{
	// Set camera pos and rotation
	//...

	// Start with 1 grid
	m_params = simParams::create(64.0f, 64, 64, 64);
}

void setupMenu::update()
{


	drawGrids();

}

bool setupMenu::setupPanel()
{
	return gridCreationPanel();
}

bool setupMenu::gridCreationPanel()
{
	// Confirming
	bool canConfirm = true; // I can't 
	{
		// First needing to do some checks. 
		// Resolution check
		float vSize = m_params.voxelSize;
		for (int i = 1; i < m_params.grids; i++) vSize /= m_params.resDecrease[i];
		if (vSize < 1.0f)
		{
			canConfirm = false;
			ImGui::Text("Can not confirm the grid: Voxelsize would be below 1.0 at the lowest resolution");
		}
		// TODO: Maybe more checks if they can not be checked earlier?
		else
		{
			ImGui::Text(" ");
		}
	}

	ImGui::BeginDisabled(!canConfirm);
	if (ImGui::Button("Confirm Grid"))
	{
		ImGui::EndDisabled();
		return true;
	}
	ImGui::EndDisabled();

	ImGui::Dummy(ImVec2(32, 32));

	ImGui::DragFloat("Main Resolution", &m_params.voxelSize, 1.0f, 1.0f, 1024.0f, "%.1f", ImGuiSliderFlags_ClampOnInput);

	// Check if possible to add new grid (we start with 1 always)
	if (m_params.grids < MAXGRIDRESOLUTIONS && m_params.grids > 0)
	{
		int resDec = 2, sizeX = 32, sizeZ = 32, offsetX = 1, offsetZ = 1;
		bool valid = true;
		{
			int parentRegion = m_params.grids == 1 ? 2 : 2 * m_params.resDecrease[m_params.grids - 1] + 2; // For main grid only 2 else, All parent ghost cells plus 2 extra cells
			sizeX = std::min(sizeX, (m_params.sizeX[m_params.grids - 1] - parentRegion) * resDec);
			sizeZ = std::min(sizeZ, (m_params.sizeZ[m_params.grids - 1] - parentRegion) * resDec);
		}
		// Make sure it is dividible
		sizeX -= sizeX % resDec;
		sizeZ -= sizeZ % resDec;
		
		// Resolution check
		float vSize = m_params.voxelSize;
		for (int i = 1; i < m_params.grids; i++) vSize /= m_params.resDecrease[i];
		vSize /= resDec;

		// Size has to support 'resDec' on both sides for the ghost cells, plus at least 'resDec' as inner cells
		if (sizeX < resDec * 2 + resDec || sizeZ < resDec * 2 + resDec)
		{
			valid = false;
			ImGui::Text("Can not create new grid: New grid can not fit in the parent grid");
		}
		else if (vSize < 1.0f)
		{
			valid = false;
			ImGui::Text("Can not create new grid: Resolution too big; size per voxel too small");
		}
		else
		{
			ImGui::Text(" ");
		}

		// Make the grid be as much in between
		if (valid)
		{
			// Divide the total space we have by 2, which becomes our offset
			offsetX = (m_params.sizeX[m_params.grids - 1] - sizeX / resDec) / 2;
			offsetZ = (m_params.sizeZ[m_params.grids - 1] - sizeZ / resDec) / 2;
		}
		ImGui::BeginDisabled(!valid);
		if (ImGui::Button("Add Grid"))
		{
			m_params.addResolution(resDec, sizeX, sizeZ, offsetX, offsetZ);
		}
		ImGui::EndDisabled();
	}

	
	for (int i = 0; i < m_params.grids; i++)
	{
		char b[32];
		snprintf(b, sizeof(b), "Grid %i", i);
		bool showImGui = false;
		// Use bool so logic can continue without showing UI
		if (ImGui::TreeNode(b)) showImGui = true;

		if (showImGui) ImGui::Text("Size of the grid");
		int maxSizeX = 1024;
		int maxSizeZ = 1024;
		int minSize = getMinimumSize(i);
		//if (i != 0) minSize = m_params.resDecrease[i] * 2 + m_params.resDecrease[i] * 2; // Ghost cells on both sides plus at least 2 in the middle
		//if (m_params.grids > i + 1) minSize += 2; // Add minimum size of child to main grid
		//int tooLittle = minSize % m_params.resDecrease[i]; // Check if we come short to correctly apply offset
		//if (tooLittle != 0) minSize += m_params.resDecrease[i] - tooLittle; // Make sure it has correct offset
		// If at i == 0, we just need 1 grid space, but further inside, we need room for ghost cells and one extra space 
		int sizeDifferenceChild = i <= 1 ? 2 : (m_params.resDecrease[i - 1] + 1) * 2;

		// x
		if (i != 0) maxSizeX = (m_params.sizeX[i - 1] - sizeDifferenceChild) * m_params.resDecrease[i];
		snprintf(b, sizeof(b), "Size X ##%i", i);
		if (showImGui) ImGui::SliderInt(b, &m_params.sizeX[i], minSize, maxSizeX, "%d", ImGuiSliderFlags_ClampOnInput);
		m_params.sizeX[i] -= m_params.sizeX[i] % m_params.resDecrease[i];
		m_params.sizeX[i] = std::max(m_params.sizeX[i], minSize);

		// y
		if (i == 0)
		{
			snprintf(b, sizeof(b), "Size Y ##%i", i);
			if (showImGui) ImGui::SliderInt(b, &m_params.sizeY[i], 1, 1024, "%d", ImGuiSliderFlags_ClampOnInput);
		}

		// z
		if (i != 0) maxSizeZ = (m_params.sizeZ[i - 1] - sizeDifferenceChild) * m_params.resDecrease[i];
		snprintf(b, sizeof(b), "Size Z ##%i", i);
		if (showImGui) ImGui::SliderInt(b, &m_params.sizeZ[i], minSize, maxSizeZ, "%d", ImGuiSliderFlags_ClampOnInput);
		m_params.sizeZ[i] -= m_params.sizeZ[i] % m_params.resDecrease[i];
		m_params.sizeZ[i] = std::max(m_params.sizeZ[i], minSize);

		if (i != 0)
		{
			int maxResDecrease = 8;
			float vSize = m_params.voxelSize;
			// Apply all division except this one and check at which value voxelsize would go below 1
			for (int j = 1; j < m_params.grids; j++) if (i != j) vSize /= m_params.resDecrease[j];
			maxResDecrease = std::min(8, std::max(2, int(std::floor(vSize))));

			snprintf(b, sizeof(b), "Resolution Increase ##%i", i);
			if (showImGui) ImGui::SliderInt(b, &m_params.resDecrease[i], 2, maxResDecrease, "%d", ImGuiSliderFlags_ClampOnInput);

			// Calculate new size per voxel
			float curVoxelSize = m_params.voxelSize;
			for (int j = 1; j <= i; j++) curVoxelSize /= m_params.resDecrease[j];
			snprintf(b, sizeof(b), "Size per voxel will be: %f", curVoxelSize);
			if (showImGui) ImGui::Text(b);


			// Offset
			// If at i == 1, we just need 1 grid space, but further inside, we need room for ghost cells and one extra space 
			int offsetDifferenceChild = i == 1 ? 1 : m_params.resDecrease[i - 1] + 1;
			int minOffset = offsetDifferenceChild;
			int maxOffsetX = m_params.sizeX[i - 1] - (m_params.sizeX[i] / m_params.resDecrease[i]) - offsetDifferenceChild;
			int maxOffsetZ = m_params.sizeZ[i - 1] - (m_params.sizeZ[i] / m_params.resDecrease[i]) - offsetDifferenceChild;
			// If too small, resize the grid
			if (maxOffsetX < minOffset) m_params.sizeX[i] = std::min(m_params.sizeX[i], maxSizeX), maxOffsetX = minOffset;
			if (maxOffsetZ < minOffset) m_params.sizeZ[i] = std::min(m_params.sizeZ[i], maxSizeZ), maxOffsetZ = minOffset;
			snprintf(b, sizeof(b), "Offset X ##%i", i);
			if (showImGui) ImGui::SliderInt(b, &m_params.offsetX[i], minOffset, maxOffsetX, "%d", ImGuiSliderFlags_ClampOnInput);
			snprintf(b, sizeof(b), "Offset Z ##%i", i);
			if (showImGui) ImGui::SliderInt(b, &m_params.offsetZ[i], minOffset, maxOffsetZ, "%d", ImGuiSliderFlags_ClampOnInput);
			// Clamp values
			m_params.offsetX[i] = std::max(minOffset, std::min(m_params.offsetX[i], maxOffsetX));
			m_params.offsetZ[i] = std::max(minOffset, std::min(m_params.offsetZ[i], maxOffsetZ));


			if (showImGui && ImGui::Button("Remove Grid"))
			{
				// Move data over by 1
				for (int j = i; j < m_params.grids; j++)
				{
					m_params.resDecrease[j] = m_params.resDecrease[j + 1];
					m_params.sizeX[j] = m_params.sizeX[j + 1];
					m_params.sizeY[j] = m_params.sizeY[j + 1];
					m_params.sizeZ[j] = m_params.sizeZ[j + 1];
					m_params.size[j] = m_params.size[j + 1];
					m_params.sizeGround[j] = m_params.sizeGround[j + 1];
					m_params.offsetX[j] = m_params.offsetX[j + 1];
					m_params.offsetZ[j] = m_params.offsetZ[j + 1];
					m_params.gridDimX[j] = m_params.gridDimX[j + 1];
					m_params.gridDimY[j] = m_params.gridDimY[j + 1];
					m_params.gridDimZ[j] = m_params.gridDimZ[j + 1];
					m_params.recalcSize();
				}
				m_params.grids--;
				// No need to reset the last index, that data will be overwritten when creating a new grid
			}
		}
		if (showImGui) ImGui::TreePop();

	}
	return false;
}

void setupMenu::drawGrids()
{
	if (m_params.grids == 0) return;

	float resDec = 1.0f;
	float prevResDec = 1.0f;
	float offsetPrevX = 0.0f;
	float offsetPrevZ = 0.0f;
	for (int i = 0; i < m_params.grids; i++)
	{
		prevResDec = resDec;
		resDec *= m_params.resDecrease[i]; // Total resolution decrease

		offsetPrevX += m_params.offsetX[i] / prevResDec;
		offsetPrevZ += m_params.offsetZ[i] / prevResDec;

		for (float x = 0; x <= m_params.sizeX[i]; x++)
		{
			float posX = (x / resDec) + offsetPrevX;
			float minPosZ = offsetPrevZ;
			float maxPosZ = m_params.sizeZ[i] / resDec + offsetPrevZ;

			bool drawFullBlue = x <= m_params.resDecrease[i] || x >= m_params.sizeX[i] - m_params.resDecrease[i];
			float bluePart = float(m_params.resDecrease[i]) / resDec;

			if (i == 0) drawFullBlue = false, bluePart = 0.0f;

			if (drawFullBlue)
			{
				CloudHub.CloudRender().shapesGLObj().AddLine(glm::vec3(posX, i * 0.01f, minPosZ), glm::vec3(posX, i * 0.01f, maxPosZ), Colors::BlueA);
			}
			else
			{
				// Draw line in 3 parts to surround the white part with blue
				CloudHub.CloudRender().shapesGLObj().AddLine(glm::vec3(posX, i * 0.01f, minPosZ), glm::vec3(posX, i * 0.01f, minPosZ + bluePart), Colors::BlueA);
				CloudHub.CloudRender().shapesGLObj().AddLine(glm::vec3(posX, i * 0.01f, minPosZ + bluePart), glm::vec3(posX, i * 0.01f, maxPosZ - bluePart), Colors::WhiteA);
				CloudHub.CloudRender().shapesGLObj().AddLine(glm::vec3(posX, i * 0.01f, maxPosZ - bluePart), glm::vec3(posX, i * 0.01f, maxPosZ), Colors::BlueA);
			}

		}

		for (float z = 0; z < m_params.sizeZ[i]; z++)
		{
			float posZ = (z / resDec) + offsetPrevZ;
			float minPosX = offsetPrevX;
			float maxPosX = m_params.sizeX[i] / resDec + offsetPrevX;

			bool drawFullBlue = z <= m_params.resDecrease[i] || z >= m_params.sizeZ[i] - m_params.resDecrease[i];
			float bluePart = float(m_params.resDecrease[i]) / resDec;

			if (i == 0) drawFullBlue = false, bluePart = 0.0f;

			if (drawFullBlue)
			{
				CloudHub.CloudRender().shapesGLObj().AddLine(glm::vec3(minPosX, i * 0.01f, posZ), glm::vec3(maxPosX, i * 0.01f, posZ), Colors::BlueA);
			}
			else
			{
				// Draw line in 3 parts to surround the white part with blue
				CloudHub.CloudRender().shapesGLObj().AddLine(glm::vec3(minPosX, i * 0.01f, posZ), glm::vec3(minPosX + bluePart, i * 0.01f, posZ), Colors::BlueA);
				CloudHub.CloudRender().shapesGLObj().AddLine(glm::vec3(minPosX + bluePart, i * 0.01f, posZ), glm::vec3(maxPosX - bluePart, i * 0.01f, posZ), Colors::WhiteA);
				CloudHub.CloudRender().shapesGLObj().AddLine(glm::vec3(maxPosX - bluePart, i * 0.01f, posZ), glm::vec3(maxPosX, i * 0.01f, posZ), Colors::BlueA);
			}
		}
	}



}

int setupMenu::getMinimumSize(int i)
{
	int minSize = 4;

	// For each child with ghost cells, add at least 2 for the ghost cells and 2 for the interior.
	if (i != 0) minSize = m_params.resDecrease[i] * 2 + m_params.resDecrease[i] * 2;

	// Loop over all child from small to big and check minimum size for each one
	float prevChildSize = 0.0f;
	for (int j = m_params.grids - 1; j > i; j--)
	{
		float childMinSize = 4 * m_params.resDecrease[j]; // Min size is ghost cells + 2 interior cells
		int ghostCells = 2 * m_params.resDecrease[j]; // Ghost cells on both sides 
		// Min size is size of the previous child + ghost cells + 2 extra
		// Then we need to grab the biggest one
		childMinSize = std::max(childMinSize, ghostCells + prevChildSize + 2);

		// Divide by resolution decrease to go to the parent
		prevChildSize = std::ceil(childMinSize / m_params.resDecrease[j]);
	}
	minSize = int(prevChildSize);
	minSize += minSize == 0 ? 4 : 2; // Add blank cells (if empty/no child, we set the default interior to be 4)
	if (i != 0) minSize += 2 * m_params.resDecrease[i]; // Add ghost cells

	int tooLittle = minSize % m_params.resDecrease[i]; // Check if we come short to correctly apply offset
	if (tooLittle != 0) minSize += m_params.resDecrease[i] - tooLittle; // Make sure it has correct offset

	return minSize;
}
