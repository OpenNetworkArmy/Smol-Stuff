#define OLC_PGE_APPLICATION

#include "olcPixelGameEngine.h"
#include <cstdint>
#include <iostream>
#include <fstream>
//#define OLC_PGEX_NT3



/*
	-start out by implementing a voxel engine using olc.
	-then a falling sand engine to simulate an environment with complex emergent behavior
	-from here read cubes into an input iteratively.
	-hook the basic_neural_net to it
	-show the training of this net
	-create a chrono construct
	-setup a matrix for an input, output, and a medium for the AI to plot it's predictions through in order to build a plan to achieve the goal
*/
class Voxel {
public:
	// State of the voxel (alive or dead)
	bool alive;
	short int neighbors;
	short int age_avg;
	//olc::Pixel Data;

	Voxel()
	{
		alive = false;
		neighbors = 0;
		age_avg = 0;
		//Data.r = 0;
		//Data.g = 0;
		//Data.b = 0;
	}

	// Function to update the state of the voxel based on its neighbors
	int update(int aliveNeighbors, int p_Rule)
	{
		int tmp_Updated = 0;

		if (neighbors != aliveNeighbors) { tmp_Updated = 1; }
		neighbors = aliveNeighbors;

		if (age_avg > 0)
		{
			age_avg--;
			tmp_Updated++;
		}



		//Data.r = 255;
		//Data.g = 255;
		//Data.b = 255;

		//Data.a = 12 * neighbors;

		// Update the state of the voxel based on the number of alive neighbors
		if (alive) 
		{
			if (age_avg < 30)
			{
				age_avg += 5;
			}
			if (aliveNeighbors < p_Rule || aliveNeighbors >(p_Rule + 1)) {
				alive = false; // die from underpopulation or overpopulation
				tmp_Updated = 1;
			}
		}
		else 
		{
			//if ((aliveNeighbors == (p_Rule + 1)) && (age_avg < 19)) 
			if ((aliveNeighbors == (p_Rule + 1)) && (age_avg < 19)) 
			{
				alive = true; // come to life from reproduction
				tmp_Updated = 1;
			}
		}
		//std::cout << "\n" << age_avg;


		return tmp_Updated;
	}
};

class VoxelSimulation {
public:
	// 2D grid of voxels
	Voxel*** grid;

	Voxel**** grid_History;
	int current_History;
	int current_History_Output;
	int size_History;

	int grid_Width;
	int grid_Height;
	int grid_Depth;

	int x_Padd;
	int y_Padd;
	int x_Shift;
	int y_Shift;

	int x_Off;
	int y_Off;

	int pixsize;

	int gameTick;

	int Rule;

	int current_X;
	int current_Y;
	int current_Z;

	bool nowalls;

	bool flg_History_Complete;

	olc::PixelGameEngine* PGE;

	VoxelSimulation(olc::PixelGameEngine* p_PGE = NULL)
	{
		Rule = 4;
		x_Padd = 10;
		y_Padd = 10;
		x_Off = 10;
		y_Off = 10;
		x_Shift = 10;
		y_Shift = 10;
		pixsize = 5;
		PGE = p_PGE;
		gameTick = 0;

		current_X = 0;
		current_Y = 0;
		current_Z = 0;

		nowalls = true;
		
		flg_History_Complete = false;


		grid_History = new Voxel * **[1000];
	}

	void set_PGE(olc::PixelGameEngine* p_PGE)
	{
		PGE = p_PGE;
	}

	// Initialize the grid
	void init(int width, int height, int depth, int p_x, int p_y)
	{
		grid_Width = width;
		grid_Height = height;
		grid_Depth = depth;

		pixsize = 5;
		x_Padd = 3;
		y_Padd = 3;
		x_Shift = grid_Width * pixsize;
		x_Shift = 4;
		y_Shift = 5;

		x_Off = p_x;
		y_Off = p_y;




		grid = new Voxel * *[grid_Width];
		for (int cou_Width = 0; cou_Width < grid_Width; cou_Width++)
		{
			grid[cou_Width] = new Voxel * [grid_Height];
			for (int cou_Height = 0; cou_Height < grid_Depth; cou_Height++)
			{
				grid[cou_Width][cou_Height] = new Voxel[grid_Depth];
			}
		}
		reset();
	}

	void resize(int p_Size)
	{
		reset_History(p_Size); //Must be done before the resize.

		for (int cou_Width = 0; cou_Width < grid_Width; cou_Width++)
		{
			for (int cou_Height = 0; cou_Height < grid_Depth; cou_Height++)
			{
				delete[] grid[cou_Width][cou_Height];
				grid[cou_Width][cou_Height] = NULL;
			}
			delete[] * grid[cou_Width];
			grid[cou_Width] = NULL;
		}

		delete[] grid;
		grid = NULL;

		grid_Width = p_Size;
		grid_Height = p_Size;
		grid_Depth = p_Size;

		grid = new Voxel * *[grid_Width];
		for (int cou_Width = 0; cou_Width < grid_Width; cou_Width++)
		{
			grid[cou_Width] = new Voxel * [grid_Height];
			for (int cou_Height = 0; cou_Height < grid_Depth; cou_Height++)
			{
				grid[cou_Width][cou_Height] = new Voxel[grid_Depth];
			}
		}
		reset();
	}

	void reset_History(int p_New_Size)
	{
		for (int cou_Index = 0; cou_Index < current_History; cou_Index++)
		{
			if (grid_History[cou_Index] != NULL)
			{
				for (int cou_Width = 0; cou_Width < size_History; cou_Width++)
				{
					for (int cou_Height = 0; cou_Height < size_History; cou_Height++)
					{
						delete[] grid_History[cou_Index][cou_Width][cou_Height];
						grid_History[cou_Index][cou_Width][cou_Height] = NULL;
					}
					delete[] * grid_History[cou_Index][cou_Width];
					grid_History[cou_Index][cou_Width] = NULL;
				}

				delete[] grid_History[cou_Index];
				grid_History[cou_Index] = NULL;
			}
		}

		size_History = p_New_Size;
		flg_History_Complete = false;
		current_History = 0;
	}

	bool step_History()
	{
		if (current_History == 300) { flg_History_Complete = true; return true; }

		grid_History[current_History] = new Voxel * *[size_History];
		for (int cou_Width = 0; cou_Width < size_History; cou_Width++)
		{
			grid_History[current_History][cou_Width] = new Voxel * [size_History];
			for (int cou_Height = 0; cou_Height < size_History; cou_Height++)
			{
				grid_History[current_History][cou_Width][cou_Height] = new Voxel[grid_Depth];
				for (int cou_Depth = 0; cou_Depth < size_History; cou_Depth++)
				{
					grid_History[current_History][cou_Width][cou_Height][cou_Depth] = grid[cou_Width][cou_Height][cou_Depth];
				}
			}
		}

		current_History++;

		if (!(current_History % 10)) { std::cout << "\n CH: " << current_History; }

		return false;
	}

	void reset()
	{
		for (int cou_Width = 0; cou_Width < grid_Width; cou_Width++)
		{
			for (int cou_Height = 0; cou_Height < grid_Depth; cou_Height++)
			{
				for (int cou_Depth = 0; cou_Depth < grid_Depth; cou_Depth++)
				{
					grid[cou_Width][cou_Height][cou_Depth].alive = 0;
				}
			}
		}

		for (int cou_Index = 0; cou_Index < 1; cou_Index++)
		{
			int ranran = rand() % (grid_Width - 21);
			for (int cou_Width = 0; cou_Width < 20; cou_Width++)
			{
				for (int cou_Height = 0; cou_Height < 20; cou_Height++)
				{
					for (int cou_Depth = 0; cou_Depth < 20; cou_Depth++)
					{
						grid[ranran + cou_Width][ranran + cou_Height][ranran + cou_Depth].alive = rand() % 2;
					}
				}
			}
		}

		reset_History(grid_Width);

		gameTick = 0;
		Rule = 4;
	}

	int get_Count(int x, int y, int z)
	{
		// Count the number of alive neighboring voxels
		int aliveNeighbors = 0;
		for (int i = -1; i <= 1; i++) {
			for (int j = -1; j <= 1; j++) {
				for (int k = -1; k <= 1; k++) {
					if (i == 0 && j == 0 && k == 0) continue; // skip current voxel

					if (nowalls)
					{
						int xi = (x + i + grid_Width) % grid_Width;
						int yj = (y + j + grid_Height) % grid_Height;
						int zk = (z + k + grid_Depth) % grid_Depth;


						if (grid[xi][yj][zk].alive)
						{
							aliveNeighbors++;
						}
					}
					else
					{
						int xi = x + i;
						int yj = y + j;
						int zk = z + k;


						if (xi >= 0 && xi < grid_Width && yj >= 0 && yj < grid_Height && zk >= 0 && zk < grid_Depth)
						{
							if (grid[xi][yj][zk].alive)
							{
								aliveNeighbors++;
							}
						}
					}
				}
			}
		}
		return aliveNeighbors;
	}

	// Update the state of all voxels in the grid
	bool update()
	{
		gameTick++;
		int depthStep = 200 / grid_Depth;
		int tmp_Updated = 0;
		int tmp_Col = 0;

		int tmp_Counter = 0;


		for (int z = 0; z < grid_Depth; z++) {
			for (int x = 0; x < grid_Width; x++) {
				for (int y = 0; y < grid_Height; y++) {


					tmp_Counter++;

					//tmp_Col = (depthStep * z);
					//grid[x][y][z].Data.g = tmp_Col;
					//grid[x][y][z].Data.b = tmp_Col;
					//grid[x][y][z].Data.r = tmp_Col;
					tmp_Updated += grid[x][y][z].update(get_Count(x, y, z), Rule);
				}
			}
		}

		if (tmp_Updated == 0) { reset(); }
		//if ((tmp_Updated > 1500) && (gameTick > 2) && (Rule < 9)) { rule_More(); gameTick = 0; }
		//if ((tmp_Updated < 250) && (gameTick > 0) && (Rule > 3)) { rule_Less(); }
		//if (gameTick > 200) { reset(); }

		return step_History();
	}


	// Update the state of all voxels in the grid
	void update_Random(int p_Count) {
		gameTick++;
		int depthStep = 250 / grid_Depth;
		int tmp_Updated = 0;
		int tmp_Col = 0;
		int tmp_X;
		int tmp_Y;
		int tmp_Z;

		for (int cou_C = 0; cou_C < p_Count; cou_C++)
		{
			tmp_X = rand() % grid_Width;
			tmp_Y = rand() % grid_Height;
			tmp_Z = rand() % grid_Depth;
			/*
			tmp_Col = (depthStep * tmp_Z) + 5;

			grid[tmp_X][tmp_Y][tmp_Z].Data.g = tmp_Col;
			grid[tmp_X][tmp_Y][tmp_Z].Data.b = tmp_Col;
			grid[tmp_X][tmp_Y][tmp_Z].Data.r = tmp_Col + grid[tmp_X][tmp_Y][tmp_Z].age_avg;
			*/
			tmp_Updated += grid[tmp_X][tmp_Y][tmp_Z].update(get_Count(tmp_X, tmp_Y, tmp_Z), Rule);
		}

		if (tmp_Updated == 0) { reset(); }

		//if ((tmp_Updated > 1500) && (gameTick > 2) && (Rule < 9)) { rule_More(); gameTick = 0; }
		//if ((tmp_Updated < 250) && (gameTick > 0) && (Rule > 3)) { rule_Less(); }

		//if (gameTick > 1000) { reset(); }

		step_History();

	}

	void output()
	{
		int tmp_XShift = 0;
		int tmp_YShift = 0;

		int tmpX[2];
		int tmpY[2];

		olc::Pixel tmp_Data;

		int depthStep = 200 / grid_Depth;
		int tmp_Col = 0;
		int tmp_Size = 0;

		Voxel tmp_Vox;

		for (int z = 0; z < grid_Depth; z++) {
			tmp_Col = depthStep * z;
			for (int x = 0; x < grid_Width; x++) {
				for (int y = 0; y < grid_Height; y++) {

					tmp_Vox = grid[x][y][z];
					if (tmp_Vox.alive)
					{
						tmp_XShift = (x_Shift * z) - (z * .05);
						tmp_YShift = (y_Shift * z) - (y * .05);
						
						tmp_Data.r = tmp_Col;
						tmp_Data.g = tmp_Col;
						tmp_Data.b = tmp_Col;
						//grid[x][y][z].Data.b = tmp_Col;
						//grid[x][y][z].Data.r = tmp_Col;

						//for (int cou_P = 0; cou_P < 2; cou_P++)
						tmp_Size = (tmp_Vox.neighbors - Rule + 1) * 3;

						//PGE->FillCircle(((x * x_Padd) + tmp_XShift + x_Off), ((y * y_Padd) + tmp_YShift + y_Off), 1, tmp_Data);
						PGE->FillRect(((x * x_Padd) + tmp_XShift + x_Off), ((y * y_Padd) + tmp_YShift + y_Off), ((tmp_Vox.neighbors - Rule) * 2), ((tmp_Vox.neighbors - Rule) * 2), tmp_Data);

						/*
						for (int cou_P = 0; cou_P < tmp_Vox.neighbors; cou_P++)
						{
							//for (int cou_Pp = 0; cou_Pp < 2; cou_Pp++)
							//FillCircle(int32_t x, int32_t y, int32_t radius, olc::Pixel p = olc::WHITE)


							//for (int cou_Pp = 0; cou_Pp < tmp_Vox.neighbors; cou_Pp++)
							{
								//PGE->Draw(((x * x_Padd) + tmp_XShift + cou_P + x_Off), ((y * y_Padd) + tmp_YShift + cou_Pp + y_Off), tmp_Data);
							}
						}
						*/
					}
				}
			}
		}
	}
	void output_History(int p_Step)
	{
		int tmp_XShift = 0;
		int tmp_YShift = 0;

		olc::Pixel tmp_Data;

		int depthStep = 200 / grid_Depth;
		int tmp_Col = 0;

		Voxel tmp_Vox;

		for (int z = 0; z < size_History; z++) {
			tmp_Col = (depthStep * z);
			for (int x = 0; x < size_History; x++) {
				for (int y = 0; y < size_History; y++) {

					tmp_Vox = grid_History[p_Step][x][y][z];
					if (tmp_Vox.alive)
					{
						tmp_XShift = (x_Shift * z) - (z * .05);
						tmp_YShift = (y_Shift * z) - (y * .05);

						tmp_Data.r = tmp_Col;
						tmp_Data.g = tmp_Col;
						tmp_Data.b = tmp_Col;

						//for (int cou_P = 0; cou_P < 2; cou_P++)


						//PGE->FillCircle(((x * x_Padd) + tmp_XShift + x_Off), ((y * y_Padd) + tmp_YShift + y_Off), ((tmp_Vox.neighbors - Rule) * 2), tmp_Data);
						PGE->FillRect(((x * x_Padd) + tmp_XShift + x_Off), ((y * y_Padd) + tmp_YShift + y_Off), ((tmp_Vox.neighbors - Rule + 1) * 2), ((tmp_Vox.neighbors - Rule + 1) * 2), tmp_Data);
						
						/*
						for (int cou_P = 0; cou_P < tmp_Vox.neighbors; cou_P++)
						{
							//for (int cou_Pp = 0; cou_Pp < 2; cou_Pp++)
							for (int cou_Pp = 0; cou_Pp < tmp_Vox.neighbors; cou_Pp++)
							{
								PGE->Draw(((x * x_Padd) + tmp_XShift + cou_P + x_Off), ((y * y_Padd) + tmp_YShift + cou_Pp + y_Off), tmp_Data);
							}
						}
						*/
					}
				}
			}
		}
	}

	int output_History_Step()
	{
		output_History(current_History_Output);

		current_History_Output = (current_History_Output + 1 + current_History) % current_History;

		return current_History_Output;
	}

	int x_Padd_Less() { return x_Padd--; }
	int x_Shift_Less() { return x_Shift--; }
	int y_Padd_Less() { return y_Padd--; }
	int y_Shift_Less() { return y_Shift--; }

	int x_Padd_More() { return x_Padd++; }
	int x_Shift_More() { return x_Shift++; }
	int y_Padd_More() { return y_Padd++; }
	int y_Shift_More() { return y_Shift++; }

	int pix_Less() { return pixsize--; }
	int pix_More() { return pixsize++; }

	int rule_Less() { return Rule--; }
	int rule_More() { return Rule++; }

	int x_Off_Less() { return x_Off--; }
	int x_Off_More() { return x_Off++; }
	int y_Off_Less() { return y_Off--; }
	int y_Off_More() { return y_Off++; }

	int no_Walls() { nowalls = true; return nowalls; }
	int yes_Walls() { nowalls = false; return nowalls; }


	void output_Con()
	{
		for (int x = 0; x < grid_Width; x++) {
			std::cout << "\n";
			for (int y = 0; y < grid_Height; y++) {
				std::cout << "\n";
				for (int z = 0; z < grid_Depth; z++) {
					std::cout << " " << grid[x][y][z].alive;
				}
			}
		}
	}
};




class Example : public olc::PixelGameEngine
{
public:

	int Current_Menu_Method;
	std::string Input;
	VoxelSimulation VoxBox;
	int Direction = 1;

	int grid_Size = 95;

	int flg_Has_Ran;

	Example()
	{
		sAppName = "Example";
		flg_Has_Ran = 0;
		//SetPixelMode(olc::Pixel::Mode::ALPHA);
	}

public:
	bool OnUserCreate() override
	{
		// Called once at the start, so create things here
		VoxBox.set_PGE(this);

		VoxBox.init(grid_Size, grid_Size, grid_Size, ((this->ScreenWidth() / 4) - (grid_Size)), ((this->ScreenHeight() / 4) - (grid_Size)));
		Current_Menu_Method = 8; //Freerun
		return true;
	}

	int tmp_X;
	int tmp_Y;
	int tmp_Num;


	bool OnUserUpdate(float fElapsedTime) override
	{

		// Handle User Input
		if (GetKey(olc::Key::A).bHeld) { std::cout << "\n x_Shift_Less " << VoxBox.x_Shift_Less(); this->Clear(olc::BLACK); }
		if (GetKey(olc::Key::D).bHeld) { std::cout << "\n x_Shift_More " << VoxBox.x_Shift_More(); this->Clear(olc::BLACK); }
		if (GetKey(olc::Key::W).bHeld) { std::cout << "\n y_Shift_Less " << VoxBox.y_Shift_Less(); this->Clear(olc::BLACK); }
		if (GetKey(olc::Key::S).bHeld) { std::cout << "\n y_Shift_More " << VoxBox.y_Shift_More(); this->Clear(olc::BLACK); }


		if (GetKey(olc::Key::LEFT).bHeld) { std::cout << "\n x_Off_Less " << VoxBox.x_Off_Less(); this->Clear(olc::BLACK); }
		if (GetKey(olc::Key::RIGHT).bHeld) { std::cout << "\n x_Off_More " << VoxBox.x_Off_More(); this->Clear(olc::BLACK); }
		if (GetKey(olc::Key::UP).bHeld) { std::cout << "\n y_Off_Less " << VoxBox.y_Off_Less(); this->Clear(olc::BLACK); }
		if (GetKey(olc::Key::DOWN).bHeld) { std::cout << "\n y_Off_More " << VoxBox.y_Off_More(); this->Clear(olc::BLACK); }

		if (GetKey(olc::Key::Q).bHeld) { std::cout << "\n x_Padd_Less " << VoxBox.x_Padd_Less(); this->Clear(olc::BLACK); }
		if (GetKey(olc::Key::E).bHeld) { std::cout << "\n x_Padd_More " << VoxBox.x_Padd_More(); this->Clear(olc::BLACK); }
		if (GetKey(olc::Key::Q).bHeld) { std::cout << "\n y_Padd_Less " << VoxBox.y_Padd_Less(); this->Clear(olc::BLACK); }
		if (GetKey(olc::Key::E).bHeld) { std::cout << "\n y_Padd_More " << VoxBox.y_Padd_More(); this->Clear(olc::BLACK); }

		if (GetKey(olc::Key::N).bHeld) { std::cout << "\n -- pix_Less " << VoxBox.pix_Less(); this->Clear(olc::BLACK); }
		if (GetKey(olc::Key::M).bHeld) { std::cout << "\n ++ pix_More " << VoxBox.pix_More(); this->Clear(olc::BLACK); }

		if (GetKey(olc::Key::O).bHeld) { std::cout << "\n -- rule_Less " << VoxBox.rule_Less(); this->Clear(olc::BLACK); }
		if (GetKey(olc::Key::P).bHeld) { std::cout << "\n ++ rule_More " << VoxBox.rule_More(); this->Clear(olc::BLACK); }

		if (GetKey(olc::Key::Z).bHeld) { grid_Size -= 1; std::cout << "\n -- grid_Size " << grid_Size; VoxBox.resize(grid_Size); this->Clear(olc::BLACK); }
		if (GetKey(olc::Key::X).bHeld) { grid_Size += 1; std::cout << "\n ++ grid_Size " << grid_Size; VoxBox.resize(grid_Size); this->Clear(olc::BLACK); }

		if (GetKey(olc::Key::F).bHeld) { std::cout << "\n Walls Off"; VoxBox.no_Walls(); this->Clear(olc::BLACK); }
		if (GetKey(olc::Key::G).bHeld) { std::cout << "\n Walls On"; VoxBox.yes_Walls(); this->Clear(olc::BLACK); }



		if (GetKey(olc::Key::R).bPressed) { VoxBox.reset(); }

		if (GetKey(olc::Key::B).bPressed) { Current_Menu_Method = 4; }

		if (GetKey(olc::Key::SPACE).bPressed) { Current_Menu_Method = 0; }


		int Step_Speed = 5;
		//Root menu
		if (Current_Menu_Method == 0)
		{
			std::cout << "\n 0: Main_Menu";
			std::cout << "\n 1: Build_Mode";
			std::cout << "\n 2: Build_Mode_Quick";
			std::cout << "\n .: ";
			std::cout << "\n .: ";
			std::cout << "\n .: ";
			std::cout << "\n .: ";
			std::cout << "\n .: ";
			std::cout << "\n 8: Freeroam/run (Press Space in window to return to console menu)";
			std::cout << "\n 9: Save()";
			std::cout << "\n 10: Load()";
			std::cout << "\n\n -->";
			Input == "";
			std::cin >> Input;
			if (Input == "0") { Current_Menu_Method = 0; }
			if (Input == "1") { Current_Menu_Method = 1; }
			if (Input == "2") { Current_Menu_Method = 2; }
			if (Input == "3") { Current_Menu_Method = 3; }
			if (Input == "4") { Current_Menu_Method = 4; }
			if (Input == "6") { Current_Menu_Method = 6; }
			if (Input == "8") { Current_Menu_Method = 8; }
			if (Input == "9") { Current_Menu_Method = 9; }
			if (Input == "10") { Current_Menu_Method = 10; }
		}
		if (Current_Menu_Method == 1)
		{
			Current_Menu_Method = 8;
		}
		if (Current_Menu_Method == 3)
		{
			Current_Menu_Method = 8;
		}
		if (Current_Menu_Method == 4)
		{
			Current_Menu_Method = 8;
		}
		if (Current_Menu_Method == 5)
		{
			Current_Menu_Method = 8;
		}
		if (Current_Menu_Method == 6)
		{
			Current_Menu_Method = 8;
		}
		if (Current_Menu_Method == 7)
		{
			Current_Menu_Method = 8;
		}
		if (Current_Menu_Method == 8)
		{
			this->Clear(olc::BLACK);
			if (!VoxBox.flg_History_Complete)
			{
				VoxBox.update();
				//VoxBox.update_Random(100);
				VoxBox.output();
			}
			if (VoxBox.flg_History_Complete)
			{
				VoxBox.output_History_Step();
			}
			
			//VoxBox.update_Random(25000);
			
		}

		if (Current_Menu_Method == 9)
		{
		}
		if (Current_Menu_Method == 10)
		{
		}



		return true;
	}
};


int main()
{
	Example demo;
	if (demo.Construct(500, 500, 1, 1))
		demo.Start();

	return 0;
}





