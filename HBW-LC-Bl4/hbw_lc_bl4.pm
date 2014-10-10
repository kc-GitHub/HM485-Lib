package HM485::Devicefile;

our %definition = (
	'HBW_LC_Bl4'	=> {
		'version'		=> 13,
		'eeprom-size'	=> 1024,
		'models'	=> {
			'HBW_LC_Bl4'	=> {
				'name'			=> 'RS485 blind actuator 4-channel',
				'type'			=> 130,
				'minFW_version'	=> 0x0303
			},
		},
		'params' => {
			'master'	=> {
				'logging_time'	=> {
					'logical'		=> {
						'type'		=> 'float',
						'min'		=> 0.1,
						'max'		=> 25.5,
						'default'	=> 2.0,
						'unit'		=> 's',
					},
					'physical'	=> {
						'type'			=> 'int',
						'size'			=> 1.0,
						'interface'		=> 'eeprom',
						'address_id'	=> 0x0001
					},
					'conversion'	=> {
						'type'		=> 'float_integer_scale',
						'factor'	=> 10,
						'offset'	=> 0.0
					}
				},
				'central_address'	=> {
					'hidden'		=> 1,
					'enforce'		=> 0x00000001,
					'logical'		=> {
						'type'		=> 'int',
					},
					'physical'	=> {
						'type'			=> 'int',
						'size'			=> 4,
						'interface'		=> 'eeprom',
						'address_id'	=> 0x0002
					}
				},
				'direct_link_deactivate'	=> {
					'hidden'		=> 1,
					'enforce'		=> 1,
					'logical'		=> {
						'type'		=> 'boolean',
						'default'	=> 0,
					},
					'physical'	=> {
						'type'			=> 'int',
						'size'			=> 0.1,
						'interface'		=> 'eeprom',
						'address_id'	=> 0x0006
					}
				},
			},
		},
		'frames'	=> {
			'level_set'	=> {
				'type'		=> 0x78,
				'dir'		=> 'to_device',
				'ch_field'	=> 10,
				'params'	=> {
					'level'		=> {
						'type'	=> 'int',
						'id'	=> 11.0,
						'size'	=> 1 
					},
				},
			},
			'level_get'	=> {
				'type'		=> 0x73,
				'dir'		=> 'to_device', 
				'ch_field'	=> 10,
			},
			'info_level'	=> {
				'type'		=> 0x69,
				'dir'		=> 'from_device',
				'event'		=> 1,
				'ch_field'	=> 10,
				'params'	=> {
					'level'		=> {
						'type'	=> 'int',
						'id'	=> 11.0,
						'size'	=> 1
					},
					'state_flags'	=> { 
						'type'	=> 'int',
						'id'	=> 12.4,
						'size'	=> 0.3
					},
				},
			},
			'stop'	=> {
				'type'		=> 0x78,
				'dir'		=> 'to_device',
				'ch_field'	=> 10,
				'params'	=> {
					'level'		=> {
						'type'			=> 'int',
						'id'			=> 11.0,
						'size'			=> 1,
						'const_value'	=> 201
					},
				},
			},
			'key_event_short'	=> {
				'type'		=> 0x4B,
				'dir'		=> 'from_device',
				'event'		=> 1,
				'ch_field'	=> 10,
				'params'	=> {
					'key'	=> {
						'type'			=> 'int',
						'id'			=> 12.0,
						'size'			=> 0.1,
						'const_value'	=> 0
					},
					'counter'	=> {
						'type'	=> 'int',
						'id'	=> 12.2,
						'size'	=> 0.6
					}
				}
			},
			'key_event_long'	=> {
				'type'		=> 0x4B,
				'dir'		=> 'from_device',
				'event'		=> 1,
				'ch_field'	=> 10,
				'params'	=> {
					'key'	=> {
						'type'			=> 'int',
						'id'			=> 12.0,
						'size'			=> 0.1,
						'const_value'	=> 1
					},
					'counter'	=> {
						'type'	=> 'int',
						'id'	=> 12.2,
						'size'	=> 0.6
					}
				}
			},
			'key_sim_short'	=> {
				'type'			=> 0x4B,										# Key-Sim frames are 0xCB? A 0x4B with set 8 bit?
				'dir'			=> 'from_device',
				'ch_field'		=> 10,
				'rec_ch_field'	=> 11, 
				'params'	=> {
					'key'	=> {
						'type'			=> 'int',
						'id'			=> 12.0,
						'size'			=> 0.1,
						'const_value'	=> 0
					},
					'sim_counter'	=> {
						'type'	=> 'int',
						'id'	=> 12.2,
						'size'	=> 0.6
					},
				},
			},
			'key_sim_long'	=> {
				'type'			=> 0x4B,
				'dir'			=> 'from_device',
				'ch_field'		=> 10,
				'rec_ch_field'	=> 11,
				'params'	=> {
					'key'	=> {
						'type'			=> 'int',
						'id'			=> 12.0,
						'size'			=> 0.1,
						'const_value'	=> 1
					},
					'sim_counter'	=> {
						'type'	=> 'int',
						'id'	=> 12.2,
						'size'	=> 0.6
					}
				}
			},
			'set_lock'	=> {
				'type'		=> 0x6C,
				'dir'		=> 'to_device',
				'ch_field'	=> 11,
				'params'	=> {
					'inhibit'	=> {
						'type'	=> 'int',
						'id'	=> 12.0,
						'size'	=> 1.0
					}
				}
			},
			'toggle_install_test'	=> {
				'type'		=> 0x78,
				'dir'		=> 'to_device',
				'ch_field'	=> 10,
				'params'	=> {
					'toggle_flag'	=> {
						'type'	=> 'int',
						'id'	=> 11.0,
						'size'	=> 1.0
					}
				}
			}
		},
		'channels'	=> {
			'maintenance' => {
				'id'		=> 0,
				'ui-flags'	=> 'internal',
				'class'		=> 'maintenance',
				'count'	=> 1,
				'params'	=> {
					'master'	=> {},
					'values'	=> {
						'unreach'	=> {
							'operations'	=> 'read,event',
							'ui-flags'		=> 'service',
							'logical'		=> {
								'type'		=> 'boolean',
							},
							'physical'		=> {
								'type'		=> 'int',
								'interface'	=> 'internal',
							},
						},
						'sticky_unreach'	=> {
							'operations'	=> 'read,write,event',
							'ui-flags'		=> 'service',
							'logical'		=> {
								'type'		=> 'boolean',
							},
							'physical'		=> {
								'type'		=> 'int',
								'interface'	=> 'internal',
							}
						},
						'config_pending'	=> {
							'operations'	=> 'read,event',
							'ui-flags'		=> 'service',
							'logical'		=> {
								'type'		=> 'boolean',
							},
							'physical'		=> {
								'type'		=> 'int',
								'interface'	=> 'internal',
							}
						}
					}
				}
			},
			'blind' => {
				'id'	=> 1,
				'count'	=> 4,
				'physical_id_offset'	=> -1,
				'link_roles'	=> {
					'target'	=> 'switch',
				},
				'params'	=> {
					'master'	=> {
						'address_start'	=> 0x07,
						'address_step'	=> 7,
						'logging'	=> {
							'logical'	=> {
								'type'	=> 'option',
								'options' 	=> 'off,on',
								'default'	=> 'on',
							},
							'physical'	=> {
								'type'			=> 'int',
								'size'			=> 0.1,
								'interface'		=> 'eeprom',
								'address_id'	=> 0.0
							}
						},
						'change_over_delay'	=> {
							'logical'	=> {
								'type'		=> 'float',
								'min' 		=> 0.0,
								'max'	 	=> 25.5,
								'default'	=> 0.5,
								'unit'		=> 's',
							},
							'conversion'	=> {
								'type'		=> 'float_integer_scale',
								'factor'	=> 10,
								'ofset'		=> 0.0,
							},
							'physical'	=> {
								'type'			=> 'int',
								'size'			=> 1,
								'interface'		=> 'eeprom',
								'address_id'	=> 1.0
							}
						},
						'reference_run_counter'	=> {
							'logical'	=> {
								'type'		=> 'integer',
								'min' 		=> 0,
								'max'	 	=> 100,
								'default'	=> 0,
							},
							'physical'	=> {
								'type'			=> 'int',
								'size'			=> 1,
								'interface'		=> 'eeprom',
								'address_id'	=> 2.0
							}
						},
						'reference_running_time_bottom_top'	=> {
							'logical'	=> {
								'type'		=> 'float',
								'min' 		=> 0.1,
								'max'	 	=> 6000,
								'default'	=> 50,
								'unit'		=> 's',
							},
							'conversion'	=> {
								'type'		=> 'float_integer_scale',
								'factor'	=> 10,
								'ofset'		=> 0.0,
							},
							'physical'	=> {
								'type'			=> 'int',
								'size'			=> 2,
								'interface'		=> 'eeprom',
								'endian'		=> 'little',
								'address_id'	=> 3.0
							}
						},
						'reference_running_time_top_bottom'	=> {
							'logical'	=> {
								'type'		=> 'float',
								'min' 		=> 0.1,
								'max'	 	=> 6000,
								'default'	=> 50,
								'unit'		=> 's',
							},
							'conversion'	=> {
								'type'		=> 'float_integer_scale',
								'factor'	=> 10,
								'ofset'		=> 0.0,
							},
							'physical'	=> {
								'type'			=> 'int',
								'size'			=> 2,
								'interface'		=> 'eeprom',
								'endian'		=> 'little',
								'address_id'	=> 5.0
							}
						}
					},
					'values' => {
						'level'	=> {
							'operations'=> 'read,write,event',
							'control'	=> 'blind.level',
							'logical'	=> {
								'type'		=> 'int',
								'default'	=> 0,
								'min'		=> 0,
								'max'		=> 100,
								'unit'		=> '100%',
							},
							'physical'	=> {
								'type'		=> 'int',
								'interface'	=> 'command',
								'value_id'	=> 'level',
								'set'	=> {
									'request'	=> 'level_set',
								},
								'get'	=> {
									'request'	=> 'level_get',
									'response'	=> 'info_level',
								},
								'event'	=> {
									'frame'	=> 'info_level',
								},
							},
							'conversion'	=> {
								'type'		=> 'float_integer_scale',
								'factor'	=> 2,
								'false'		=> 0,
								'true'		=> 200
							},
							'value_map'	=> {
								'type'	=> 'integer_integer_map',
								'01'	=> {
									'device_value'		=> 0x04,
									'parameter_value'	=> 1,
									'mask'				=> 0x04,
								},
								'02'	=> {
									'device_value'		=> 0x00,
									'parameter_value'	=> 0,
								},
								'03'	=> {
									'device_value'		=> 0x01,
									'parameter_value'	=> 1,
								},
								'04'	=> {
									'device_value'		=> 0x02,
									'parameter_value'	=> 1,
								},
								'05'	=> {
									'device_value'		=> 0x03,
									'parameter_value'	=> 0,
								}
							}
						},
						'working' => {
							'operations'=> 'read,event',
							'ui_flags'	=> 'internal',
							'logical'	=> {
								'type'	=> 'boolean',
								'default'	=> 0,
							},
							'physical'	=> {
								'type'		=> 'int',
								'interface'	=> 'command',
								'value_id'	=> 'state_flags',
								'get'	=> {
									'request'	=> 'level_get',
									'response'	=> 'info_level',
								},
								'event'	=> {
									'frame'	=> 'info_level, ack_status',
								},
							},
							'conversion'	=> {
								'type'		=> 'boolean_integer',
							}
						},
						'direction'	=> {
							'operations'=> 'read,event',
							'ui_flags'	=> 'internal',
							'logical'	=> {
								'type'		=> 'option',
								'options' 	=> 'none, up, down, undefined',
								'default'	=> 'none',
							},
							'physical'	=> {
								'type'		=> 'int',
								'interface'	=> 'command',
								'value_id'	=> 'state_flags',
								'get'	=> {
									'request'	=> 'level_get',
									'response'	=> 'info_level',
								},
								'event'	=> {
									'frame'	=> 'info_level, ack_status',
								},
							},
							'value_map'	=> {
								'type'	=> 'option_integer',
								'01'	=> {
									'device_value'		=> 0x00,
									'parameter_value'	=> 0,
								},
								'02'	=> {
									'device_value'		=> 0x01,
									'parameter_value'	=> 1,
								},
								'03'	=> {
									'device_value'		=> 0x02,
									'parameter_value'	=> 2,
								},
								'04'	=> {
									'device_value'		=> 0x03,
									'parameter_value'	=> 3,
								}
							}
						},
						'stop'	=> {
							'operations'=> 'write',
							'control'	=> 'blind.stop',
							'logical'	=> {
								'type'		=> 'action',
							},
							'physical'	=> {
								'type'		=> 'int',
								'interface'	=> 'command',
								'value_id'	=> 'dummy',
								'set'	=> {
									'request'	=> 'stop',
								},
							},
						},
						'inhibit' => {
							'operations'=> 'read,write,event',
							'control'	=> 'none',
							'loopback'	=> 1,
							'logical'	=> {
								'type'	=> 'boolean',
								'default'	=> 0,
							},
							'physical'	=> {
								'type'		=> 'int',
								'interface'	=> 'command',
								'value_id'	=> 'inhibit',
								'set'	=> {
									'request'	=> 'set_lock',
								}
							}
						},
						'install_test' => {
							'operations'=> 'write',
							'ui_flags'	=> 'internal',
							'logical'	=> {
								'type'	=> 'action',
							},
							'physical'	=> {
								'type'		=> 'int',
								'interface'	=> 'command',
								'value_id'	=> 'toggle_flag',
								'no_init'	=> 'true',
								'set'	=> {
									'request'	=> 'toggle_install_test',
								}
							},
							'conversion'	=> {
								'type'		=> 'blind_test',
								'value'	=> 201
							}
						}
					}
				}
			}
		}
	}
);

1;