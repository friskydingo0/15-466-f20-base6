#include "PlayMode.hpp"

#include "DrawLines.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "hex_dump.hpp"
#include "load_save_png.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <random>

#define logmsg std::cout

std::vector< PlayMode::Texture > images;
std::vector< PlayMode::ImageQuestion > questions;

Load< std::string > image_str(LoadTagDefault, []() -> std::string const * {	// this load is totally fake. It's actually loading the pngs in the image list file. Couldn't figure out how to do it better
	
	std::string *ret = new std::string();
	std::string line;
	
	glm::uvec2 size;
	std::vector< glm::u8vec4 > data;

	// load image for each line in the data file
	std::ifstream img_data(data_path("round-info.txt"));	// reading lines based on https://stackoverflow.com/a/7868998
	while (img_data >> line)
	{
		logmsg << line << std::endl;

		PlayMode::ImageQuestion img_question;
		// string split based on - https://stackoverflow.com/a/14266139
		size_t pos = 0;
		std::string token;
		while ((pos = line.find("|")) != std::string::npos) {
			token = line.substr(0, pos);
			
			if (img_question.file_name.length() == 0)
				img_question.file_name = token;
			else
			{
				img_question.choices.emplace_back(token);
			}
			line.erase(0, pos + 1);
		}
		
		logmsg << "Loading image : " << img_question.file_name << std::endl;
		load_png(data_path("images/"+img_question.file_name), &size, &data, LowerLeftOrigin);
		images.emplace_back(PlayMode::Texture(size, data));
		questions.emplace_back(img_question);
	}

	img_data.close();
	return ret;
});

Load< Sound::Sample > bgm_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("sounds/bgm.wav"));
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
			return true;
		} else if (evt.key.keysym.sym == SDLK_2) {
			choice = 2;
			is_dirty = true;
			two.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_3) {
			choice = 3;
			is_dirty = true;
			three.pressed = true;
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
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	
	if (is_dirty) {
		//queue data for sending to server:
		//send a 2-byte message of type 'c':
		client.connections.back().send('c');
		client.connections.back().send(is_dirty ? 'y' : 'n');
		logmsg << "Sending choice " << std::to_string(choice) << std::endl;
		client.connections.back().send(choice);

		is_dirty = false;
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
				int buffer_size = 2;

				bool new_data = (c->recv_buffer[1] == 'y');
				if (new_data)
				{
					// show new question
					uint8_t round_num = (uint8_t)(c->recv_buffer[2]);
					uint8_t quest_num = (uint8_t)(c->recv_buffer[3]);
					prev_result = c->recv_buffer[4] == 'w' ? "Soul mates!" : "Incompatible!";
					buffer_size = 5;

					update_question(quest_num);
				}

				//and consume this part of the buffer:
				c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + buffer_size);
			}
		}
	}, 0.0);
	
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {

	//---- compute vertices to draw ----

	//vertices will be accumulated into this list and then uploaded+drawn at the end of this function:
	std::vector< Vertex > vertices;

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

	draw_rectangle(glm::vec2(drawable_size.x * 0.5f, drawable_size.y - 200.0f), glm::vec2(200.0f, 200.0f), glm::u8vec4(0xff, 0xff, 0xff, 0xff));

	
	glClearColor(0.01f, 0.01f, 0.01f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	
	//use alpha blending:
	// glEnable(GL_BLEND); // This is causing issues. Disabled for the time being
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//don't use the depth test:
	glDisable(GL_DEPTH_TEST);

	//upload vertices to vertex_buffer:
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //set vertex_buffer as current
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STATIC_DRAW); //upload vertices array
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glUseProgram(color_texture_program.program);

	glm::mat4 projection = glm::ortho(0.0f, (float)drawable_size.x, 0.0f, (float)drawable_size.y);
	glUniformMatrix4fv(color_texture_program.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(projection));

	glBindVertexArray(vertex_buffer_for_color_texture_program);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, png_tex);
	glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size()));

	glBindTexture(GL_TEXTURE_2D, 0);

	glBindVertexArray(0);
	glUseProgram(0);


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

		if (game_started)
		{
			draw_text(glm::vec2(-aspect + 0.1f, 0.0f), "1. " + questions[current_quest].choices[0], 0.09f);
			draw_text(glm::vec2(-aspect + 0.1f, -0.1f), "2. " + questions[current_quest].choices[1], 0.09f);
			draw_text(glm::vec2(-aspect + 0.1f, -0.2f), "3. " + questions[current_quest].choices[2], 0.09f);

			draw_text(glm::vec2(-aspect + 0.1f,-0.9f), "Choose your answer with (1, 2 or 3) : " + choice, 0.09f);

			draw_text(glm::vec2(aspect - 0.5f, -0.2f), prev_result, 0.09f);
			
		}
		else
		{
			draw_text(glm::vec2(-aspect + 0.1f,-0.9f), "Wait for another player to join...", 0.09f);
		}
		
	}
	GL_ERRORS();
}

void PlayMode::update_texture(int new_tex_index) {
	// https://community.khronos.org/t/changing-reload-or-unbinding-textures/55833/4
	glBindTexture(GL_TEXTURE_2D, png_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, images[new_tex_index].size.x, images[new_tex_index].size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, images[new_tex_index].data.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void PlayMode::update_question(int new_index) {
	if (current_quest == -1) {
		//Also start game
		game_started = true;

		bgm_loop = Sound::loop_3D(*bgm_sample, 1.0f, glm::vec3(0.0f, 0.0f, 0.0f), 0.0f);
		Sound::listener.position = glm::vec3(0.0f, 0.0f, 0.0f);
	}
	current_quest = (int)new_index;
	update_texture(new_index);
}