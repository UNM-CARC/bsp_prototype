#!/bin/bash

#Par_duration=0s Par_noise=noiseless ./get_sim_info_noise_profile_all.sh
#Par_duration=0s Par_noise=predata ./get_sim_info_noise_profile_all.sh
#Par_duration=0s Par_noise=bonds ./get_sim_info_noise_profile_all.sh
#Par_duration=0s Par_noise=edf ./get_sim_info_noise_profile_all.sh

Par_duration=100us Par_noise=noiseless ./get_sim_info_noise_profile_all.sh
Par_duration=100us Par_noise=predata ./get_sim_info_noise_profile_all.sh
Par_duration=100us Par_noise=bonds ./get_sim_info_noise_profile_all.sh
Par_duration=100us Par_noise=edf ./get_sim_info_noise_profile_all.sh

Par_duration=1ms Par_noise=noiseless ./get_sim_info_noise_profile_all.sh
Par_duration=1ms Par_noise=predata ./get_sim_info_noise_profile_all.sh
Par_duration=1ms Par_noise=bonds ./get_sim_info_noise_profile_all.sh
Par_duration=1ms Par_noise=edf ./get_sim_info_noise_profile_all.sh

Par_duration=10ms Par_noise=noiseless ./get_sim_info_noise_profile_all.sh
#Par_duration=10ms Par_noise=predata ./get_sim_info_noise_profile_all.sh
Par_duration=10ms Par_noise=bonds ./get_sim_info_noise_profile_all.sh
Par_duration=10ms Par_noise=edf ./get_sim_info_noise_profile_all.sh

Par_duration=100ms Par_noise=noiseless ./get_sim_info_noise_profile_all.sh
Par_duration=100ms Par_noise=predata ./get_sim_info_noise_profile_all.sh
Par_duration=100ms Par_noise=bonds ./get_sim_info_noise_profile_all.sh
Par_duration=100ms Par_noise=edf ./get_sim_info_noise_profile_all.sh

Par_duration=1s Par_noise=noiseless ./get_sim_info_noise_profile_all.sh
Par_duration=1s Par_noise=predata ./get_sim_info_noise_profile_all.sh
Par_duration=1s Par_noise=bonds ./get_sim_info_noise_profile_all.sh
Par_duration=1s Par_noise=edf ./get_sim_info_noise_profile_all.sh

Par_duration=10s Par_noise=noiseless ./get_sim_info_noise_profile_all.sh
Par_duration=10s Par_noise=predata ./get_sim_info_noise_profile_all.sh
Par_duration=10s Par_noise=bonds ./get_sim_info_noise_profile_all.sh
Par_duration=10s Par_noise=edf ./get_sim_info_noise_profile_all.sh

