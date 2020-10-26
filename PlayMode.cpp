#include "PlayMode.hpp"

#include "DrawLines.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "hex_dump.hpp"
#include "load_save_png.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <random>

#define log std::cout

std::vector< PlayMode::Texture > images;
Load< std::string > image_str(LoadTagDefault, []() -> std::string const * {	// this load is totally fake. It's actually loading the pngs in the image list file. Couldn't figure out how to do it better
	
	std::string *ret = new std::string();
	std::string img_name;
	
	glm::uvec2 size;
	std::vector< glm::u8vec4 > data;
	// load image for each line in the data file
	std::ifstream img_data(data_path("img-list"));	// reading lines based on https://stackoverflow.com/a/7868998
	while (img_data >> img_name)
	{
		log << "Loading " << img_name << std::endl;
		load_png(data_path("images/"+img_name), &size, &data, LowerLeftOrigin);
		images.emplace_back(PlayMode::Texture(size, data));
	}
	img_data.close();
	return ret;
});

PlayMode::PlayMode(Client &client_) : client(client_) {
	// from game0 base code
	glGenBuffers(1, &vertex_buffer);
	glGenBuffers(1, &element_buffer);

	{ //vertex array mapping buffer for color_texture_program:
		//ask OpenGL to fill vertex_buffer_for_color_texture_program with the name of an unused vertex array object:
		glGenVertexArrays(1, &vertex_buffer_for_color_texture_program);

		//set vertex_buffer_for_color_texture_program as the current vertex array object:
		glBindVertexArray(vertex_buffer_for_color_texture_program);

		//set vertex_buffer as the source of glVertexAttribPointer() commands:
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		//set up the vertex array object to describe arrays of PongMode::Vertex:
		glVertexAttribPointer(
			color_texture_program.Position_vec4, //attribute
			3, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 0 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Position_vec4);
		//[Note that it is okay to bind a vec3 input to a vec4 attribute -- the w component will be filled with 1.0 automatically]

		glVertexAttribPointer(
			color_texture_program.Color_vec4, //attribute
			4, //size
			GL_UNSIGNED_BYTE, //type
			GL_TRUE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Color_vec4);

		glVertexAttribPointer(
			color_texture_program.TexCoord_vec2, //attribute
			2, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 + 4*1 //offset
		);
		glEnableVertexAttribArray(color_texture_program.TexCoord_vec2);

		//done referring to vertex_buffer, so unbind it:
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//done setting up vertex array object, so unbind it:
		glBindVertexArray(0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //assign texture:
		
		//ask OpenGL to fill white_tex with the name of an unused texture object:
		glGenTextures(1, &png_tex);

		//bind that texture object as a GL_TEXTURE_2D-type texture:
		glBindTexture(GL_TEXTURE_2D, png_tex);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, images[0].size.x, images[0].size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, images[0].data.data());

		//set filtering and wrapping parameters:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//since texture uses a mipmap and we haven't uploaded one, instruct opengl to make one for us:
		glGenerateMipmap(GL_TEXTURE_2D);

		//Okay, texture uploaded, can unbind it:
		glBindTexture(GL_TEXTURE_2D, 0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}
}

PlayMode::~PlayMode() {
	//----- free OpenGL resources -----
	glDeleteBuffers(1, &vertex_buffer);
	vertex_buffer = 0;

	glDeleteVertexArrays(1, &vertex_buffer_for_color_texture_program);
	vertex_buffer_for_color_texture_program = 0;

	glDeleteTextures(1, &png_tex);
	png_tex = 0;
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.repeat) {
			//ignore repeats
		} else if (evt.key.keysym.sym == SDLK_1) {
			choice = 1;
			is_dirty = true;
			one.pressed = true;
			glBindTexture(GL_TEXTURE_2D, png_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, images[choice].size.x, images[choice].size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, images[choice].data.data());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			//since texture uses a mipmap and we haven't uploaded one, instruct opengl to make one for us:
			glGenerateMipmap(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);
			return true;
		} else if (evt.key.keysym.sym == SDLK_2) {
			choice = 2;
			is_dirty = true;
			two.pressed = true;
			glBindTexture(GL_TEXTURE_2D, png_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, images[choice].size.x, images[choice].size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, images[choice].data.data());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			//since texture uses a mipmap and we haven't uploaded one, instruct opengl to make one for us:
			glGenerateMipmap(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);
			return true;
		} else if (evt.key.keysym.sym == SDLK_3) {
			choice = 3;
			is_dirty = true;
			three.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_4) {
			choice = 4;
			is_dirty = true;
			four.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_1) {
			one.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_2) {
			two.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_3) {
			three.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_4) {
			four.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	
	//queue data for sending to server:
	//send a 2-byte message of type 'c':
	client.connections.back().send('c');
	client.connections.back().send(is_dirty ? 'y' : 'n');

	if (is_dirty) {
		log << "Sending choice " << std::to_string(choice) << std::endl;
		client.connections.back().send(choice);
	}

	//send/receive data:
	client.poll([this](Connection *c, Connection::Event event){
		if (event == Connection::OnOpen) {
			std::cout << "[" << c->socket << "] opened" << std::endl;
		} else if (event == Connection::OnClose) {
			std::cout << "[" << c->socket << "] closed (!)" << std::endl;
			throw std::runtime_error("Lost connection to server!");
		} else { assert(event == Connection::OnRecv);
			std::cout << "[" << c->socket << "] recv'd data. Current buffer:\n" << hex_dump(c->recv_buffer); std::cout.flush();
			//expecting message(s) like 'm' + 3-byte length + length bytes of text:
			while (c->recv_buffer.size() >= 2) {
				std::cout << "[" << c->socket << "] recv'd data. Current buffer:\n" << hex_dump(c->recv_buffer); std::cout.flush();
				char type = c->recv_buffer[0];
				if (type != 'm') {
					throw std::runtime_error("Server sent unknown message type '" + std::to_string(type) + "'");
					break;
				}

				// bool time_left = (bool)c->recv_buffer[1];
				// uint8_t round_num = c->recv_buffer[2];
				server_message = "Msgc->recv_buffer: " + std::to_string(c->recv_buffer[1]);

				// uint32_t size = (
					// (uint32_t(c->recv_buffer[1]) << 16) | (uint32_t(c->recv_buffer[2]) << 8) | (uint32_t(c->recv_buffer[3]))
				// );
				// if (c->recv_buffer.size() < 4 + size) break; //if whole message isn't here, can't process
				//whole message *is* here, so set current server message:
				// server_message = std::string(c->recv_buffer.begin() + 4, c->recv_buffer.begin() + 4 + size);

				//and consume this part of the buffer:
				c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 2);
			}
		}
	}, 0.0);

	// choice = 0;
	is_dirty = false;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {

	//---- compute vertices to draw ----

	//vertices will be accumulated into this list and then uploaded+drawn at the end of this function:
	std::vector< Vertex > vertices;
	unsigned int indices[] = {  
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

	//inline helper function for rectangle drawing:
	auto draw_rectangle = [&vertices](glm::vec2 const &center, glm::vec2 const &radius, glm::u8vec4 const &color) {
		//draw rectangle as two CCW-oriented triangles:
		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.0f, 0.0f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y-radius.y, 0.0f), color, glm::vec2(1.0f, 0.0f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(1.0f, 1.0f));

		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.0f, 0.0f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(1.0f, 1.0f));
		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.0f, 1.0f));
	};

	draw_rectangle(glm::vec2(drawable_size.x * 0.5f, drawable_size.y * 0.5f), glm::vec2(100.0f, 100.0f), glm::u8vec4(0xff, 0xff, 0xff, 0xff));

	
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	
	//use alpha blending:
	// glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//don't use the depth test:
	glDisable(GL_DEPTH_TEST);

	//upload vertices to vertex_buffer:
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //set vertex_buffer as current
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STATIC_DRAW); //upload vertices array
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//set color_texture_program as current program:
	glUseProgram(color_texture_program.program);

	glm::mat4 projection = glm::ortho(0.0f, (float)drawable_size.x, 0.0f, (float)drawable_size.y);
	glUniformMatrix4fv(color_texture_program.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(projection));

	//use the mapping vertex_buffer_for_color_texture_program to fetch vertex data:
	glBindVertexArray(vertex_buffer_for_color_texture_program);

	//bind the solid white texture to location zero so things will be drawn just with their colors:
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, png_tex);
	
	GL_ERRORS();
	//run the OpenGL pipeline:
	glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size()));

	//unbind the solid white texture:
	glBindTexture(GL_TEXTURE_2D, 0);

	//reset vertex array to none:
	glBindVertexArray(0);

	//reset current program to none:
	glUseProgram(0);
	

	GL_ERRORS(); //PARANOIA: print errors just in case we did something wrong.




	// glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	// glClear(GL_COLOR_BUFFER_BIT);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		auto draw_text = [&](glm::vec2 const &at, std::string const &text, float H) {
			lines.draw_text(text,
				glm::vec3(at.x, at.y, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			float ofs = 2.0f / drawable_size.y;
			lines.draw_text(text,
				glm::vec3(at.x + ofs, at.y + ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		};

		draw_text(glm::vec2(-aspect + 0.1f, 0.0f), server_message, 0.09f);

		draw_text(glm::vec2(-aspect + 0.1f,-0.9f), "(press WASD to change your total)", 0.09f);
	}
	GL_ERRORS();
}
