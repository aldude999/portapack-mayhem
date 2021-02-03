/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * 
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "proc_mictx.hpp"
#include "portapack_shared_memory.hpp"
#include "sine_table_int8.hpp"
#include "tonesets.hpp"
#include "event_m4.hpp"

#include <cstdint>

void MicTXProcessor::beep_check(const bool enable_beep){

	}

void MicTXProcessor::execute(const buffer_c8_t& buffer){

	// This is called at 1536000/2048 = 750Hz
	
	if (!configured) return;
	
	audio_input.read_audio_buffer(audio_buffer);

	if (usb_enabled == true || lsb_enabled == true) { 
	// SSB modified from strijar's code
		for (size_t counter = 0; counter < buffer.count; counter++) {
		if (play_beep) {
			if (beep_timer) {
				beep_timer--;
			} else {
				beep_timer = baseband_fs * 0.05;			// 50ms
				
				if (beep_index == BEEP_TONES_NB) {
					configured = false;
					shared_memory.application_queue.push(txprogress_message);
				} else {
					beep_gen.configure(beep_deltas[beep_index], 1.0);
					beep_index++;
				}
			}
			
			sample = beep_gen.process(0);
		}
			if (counter % 192 == 0) {
				float	i = 0.0, q = 0.0;
				if (!play_beep) {
		    			sample = audio_buffer.p[counter >> 6] >> 2; // 1536000 / 64 = 24000
					sample *= audio_gain;
				} 
		
				//SSB
				if ( usb_enabled == true)
					hilbert.execute(sample / 32768.0f, q, i);
				if ( lsb_enabled == true)
					hilbert.execute(sample / 32768.0f, i, q);
				i *= 127.0f;
				q *= 127.0f;
				
				re = q;		im = i;	
			
			
			}					
			buffer.p[counter] = { re, im};//im };
		}
			
	}
	else if (fm_enabled) {
	for (size_t i = 0; i < buffer.count; i++) {
		
		if (!play_beep) {

				sample = audio_buffer.p[i >> 6] >> 8; // 1536000 / 64 = 24000
				sample *= audio_gain;
			
			power_acc += (sample < 0) ? -sample : sample;	// Power average for UI vu-meter
			
			if (power_acc_count) {
				power_acc_count--;
			} else {
				power_acc_count = divider;
				level_message.value = power_acc / (divider / 4);	// Why ?
				shared_memory.application_queue.push(level_message);
				power_acc = 0;
			}
		} else {
			if (beep_timer) {
				beep_timer--;
			} else {
				beep_timer = baseband_fs * 0.05;			// 50ms
				
				if (beep_index == BEEP_TONES_NB) {
					configured = false;
					shared_memory.application_queue.push(txprogress_message);
				} else {
					beep_gen.configure(beep_deltas[beep_index], 1.0);
					beep_index++;
				}
			}
			
			sample = beep_gen.process(0);
		}
		
		sample = tone_gen.process(sample);
		
		// FM
			delta = sample * fm_delta;
			
			phase += delta;
			sphase = phase >> 24;

			re = (sine_table_i8[(sphase + 64) & 255]);
			im = (sine_table_i8[sphase]);
			buffer.p[i] = { re, im };
		}
		
		}
		else if (am_enabled == true) {
		
		// AM
		for (size_t counter = 0; counter < buffer.count; counter++) {
		if (play_beep) {
			if (beep_timer) {
				beep_timer--;
			} else {
				beep_timer = baseband_fs * 0.05;			// 50ms
				
				if (beep_index == BEEP_TONES_NB) {
					configured = false;
					shared_memory.application_queue.push(txprogress_message);
				} else {
					beep_gen.configure(beep_deltas[beep_index], 1.0);
					beep_index++;
				}
			}
			
			sample = beep_gen.process(0);
		}
			if (counter % 192 == 0) {
				float q = 0.0;
				if (!play_beep) {
		    			sample = audio_buffer.p[counter >> 6] >> 2; // 1536000 / 64 = 24000
					sample *= audio_gain;
				} 

		
				//AM
					q = sample / 32768.0f;
				q *= 127.0f;
				
				re = q + am_carrier_lvl;		im = q + am_carrier_lvl;	
			
			
			}					
			buffer.p[counter] = { re, im};//im };
		}
		
			 //AM TEST CODE
			//sphase = fm_delta >> 24;
			/*if (am_moddiv_lvl > 13) {
				am_sample = (1 + (sample));//1 + (sample * (am_moddiv_lvl - 12)));  // * (am_moddiv_lvl - 12)));
			} else {*/
			
			//if (i % 64 == 0) { //AM Low pass filter
			//
			//		if (am_moddiv_lvl == 0) 
			//			am_sample = (sample * 0.5f); //add a divide for 0, 0 is bad.
			//		else
			//			am_sample = (sample * (am_moddiv_lvl));//1 + (sample / ( 13 - am_moddiv_lvl)));    /// ( 13 - am_moddiv_lvl)));
					//}
						
			//}
			//re = (am_sample >> 24) + am_carrier_lvl;//sine_table_i8[am_sample >> 24];// - 0) * (127 - 0)) / (0xFFFFFFFF - 0) + -127); //* cos_table_int8[(sphase+64) & 255]; // Q calc = carrier level + sample x cosine(time inverted)
			//im = (am_sample >> 24) + am_carrier_lvl;//sine_table_i8[am_sample >> 24];//(1 + (sample / 25));//(1 + sample) * cos_table_int8[sphase]; // Q calcc = carrier level + sample * cosine(time)

			//else {
			//	re = am_carrier_lvl;
			//	im = am_carrier_lvl;
			//}
			



		} else {
			re = 0;
			im = 0;
			//buffer.p[counter] = { re, im};
		}
		
		
		}
		
	


void MicTXProcessor::on_message(const Message* const msg) {
	const AudioTXConfigMessage config_message = *reinterpret_cast<const AudioTXConfigMessage*>(msg);
	const RequestSignalMessage request_message = *reinterpret_cast<const RequestSignalMessage*>(msg);
	
	switch(msg->id) {
		case Message::ID::AudioTXConfig:
			fm_delta = config_message.deviation_hz * (0xFFFFFFUL / baseband_fs);
			
			audio_gain = config_message.audio_gain;
			divider = config_message.divider;

			//AM Settings
			am_carrier_lvl = config_message.am_carrier_level;
			am_moddiv_lvl = config_message.am_modulation_divider;
			am_enabled = config_message.am_enabled;
			if (am_enabled || usb_enabled || lsb_enabled) {
				fm_enabled = false;
			}
			usb_enabled = config_message.usb_enabled;
			lsb_enabled = config_message.lsb_enabled;
			if (usb_enabled || lsb_enabled)
				am_enabled == false; fm_enabled == false;

			power_acc_count = 0;
			
			tone_gen.configure(config_message.tone_key_delta, config_message.tone_key_mix_weight);
			
			txprogress_message.done = true;

			play_beep = false;
			configured = true;
			break;
		
		case Message::ID::RequestSignal:
			if (request_message.signal == RequestSignalMessage::Signal::BeepRequest) {
				beep_index = 0;
				beep_timer = 0;
				play_beep = true;
			}
			break;

		default:
			break;
	}
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<MicTXProcessor>() };
	event_dispatcher.run();
	return 0;
}
