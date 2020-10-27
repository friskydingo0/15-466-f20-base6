#include "Mode.hpp"

#include "Connection.hpp"
#include "ColorTextureProgram.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode(Client &client);
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	// list of all the images and choices
	struct ImageQuestion
	{
		std::string file_name;
		std::vector< std::string > choices;
	};
	

	// loaded texture references
	struct Texture {
		Texture(glm::uvec2 const &size_, std::vector< glm::u8vec4 > const &data_) :
			size(size_), data(data_) {}
		glm::uvec2 size;
		std::vector< glm::u8vec4 > data;
	};
	

	// from game 0 base code
	
	//draw functions will work on vectors of vertices, defined as follows:
	struct Vertex {
		Vertex(glm::vec3 const &Position_, glm::u8vec4 const &Color_, glm::vec2 const &TexCoord_) :
			Position(Position_), Color(Color_), TexCoord(TexCoord_) { }
		glm::vec3 Position;
		glm::u8vec4 Color;
		glm::vec2 TexCoord;
	};
	static_assert(sizeof(Vertex) == 4*3 + 1*4 + 4*2, "PongMode::Vertex should be packed");
	
	//Shader program that draws transformed, vertices tinted with vertex colors:
	ColorTextureProgram color_texture_program;

	//Buffer used to hold vertex data during drawing:
	GLuint vertex_buffer = 0;

	//Buffer for elements
	GLuint element_buffer = 0;

	//Vertex Array Object that maps buffer locations to color_texture_program attribute locations:
	GLuint vertex_buffer_for_color_texture_program = 0;

	//Solid white texture:
	GLuint png_tex = 0;

	//input tracking:
	struct Button {
		uint8_t pressed = 0;
	} one, two, four, three;

	bool game_started = false;

	// choice var
	uint8_t choice = 0;

	int current_quest = -1;
	std::string prev_result;

	// is game state dirty (changed)
	bool is_dirty = false;

	//last message from server:
	std::string server_message;

	//connection to server:
	Client &client;

	std::shared_ptr< Sound::PlayingSample > bgm_loop;

	// methods
	void update_texture(int new_tex_index);

	void update_question(int index);

};
