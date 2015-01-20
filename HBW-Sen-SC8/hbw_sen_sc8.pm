package HM485::Devicefile;

our %definition = (
	'HBW_Sen_SC8'	=> {
		'version'		=> 12,
		'eeprom-size'	=> 1024,
		'models'	=> {
			'HBW_Sen_SC8'	=> {
				'name'			=> 'RS485 Homebrew Tastermodul (8fach)',
				'type'			=> 134,
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
				}
			}
		},
		'frames'	=> {
			'level_set'	=> {
				'type'		=> 0x78,
				'dir'		=> 'to_device',
				'ch_field'	=> 10,
				'params'	=> {
					'state'		=> {
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
					'state'		=> {
						'type'	=> 'int',
						'id'	=> 11.0,
						'size'	=> 4
					},
					'state_flags'	=> { 
						'type'	=> 'int',
						'id'	=> 12.4,
						'size'	=> 0.3
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
						'size'			=> 0.2,
						'const_value'	=> 2
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
						'size'			=> 0.2,
						'const_value'	=> 3
					},
					'counter'	=> {
						'type'	=> 'int',
						'id'	=> 12.2,
						'size'	=> 0.6
					}
				}
			},
			'key_event_longrelease'	=> {
				'type'		=> 0x4B,
				'dir'		=> 'from_device',
				'event'		=> 1,
				'ch_field'	=> 10,
				'params'	=> {
					'key'	=> {
						'type'			=> 'int',
						'id'			=> 12.0,
						'size'			=> 0.2,
						'const_value'	=> 1
					},
					'counter'	=> {
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
			'key'	=> {
				'id'	=> 1,
				'count'	=> 8,
				'physical_id_offset'	=> -1,
				'link_roles'	=> {
					'source'	=> 'switch',
				},
				'params'	=> {
					'master'	=> {
						'address_start'	=> 0x07,
						'address_step'	=> 4,
						'input_locked'	=> {
							'logical'	=> {
								'type'	=> 'boolean',
								'default'	=> 0,
							},
							'physical'	=> {
								'type'			=> 'int',
								'size'			=> 0.1,
								'interface'		=> 'eeprom',
								'address_id'	=> 0.0
							},
							'conversion'	=> {
								'type'		=> 'boolean_integer',
								'invert'	=> 0
							}
						},
						'inverted'	=> {
							'logical'	=> {
								'type'	=> 'boolean',
								'default'	=> 1,
							},
							'physical'	=> {
								'type'			=> 'int',
								'size'			=> 0.1,
								'interface'		=> 'eeprom',
								'address_id'	=> 1.0
							},
							'conversion'	=> {
								'type'		=> 'boolean_integer',
								'invert'	=> 0
							}
						},
						'pullup'	=> {
							'logical'	=> {
								'type'	=> 'boolean',
								'default'	=> 1,
							},
							'physical'	=> {
								'type'			=> 'int',
								'size'			=> 0.1,
								'interface'		=> 'eeprom',
								'address_id'	=> 2.0
							},
							'conversion'	=> {
								'type'		=> 'boolean_integer',
								'invert'	=> 0
							}
						},
						'long_press_time'	=> {
							'logical'	=> {
								'type'		=> 'float',
								'min'		=> 0.4,
								'max'		=> 5,
								'default'	=> 1.0,
								'unit'		=> 's',
							},
							'physical'	=> {
								'type'			=> 'int',
								'size'			=> 1.0,
								'interface'		=> 'eeprom',
								'address_id'	=> 3.0
							},
							'conversion'	=> {
								'type'		=> 'float_integer_scale',
								'factor'	=> 10,
								'value_map'	=> {
									'type'	=> 'integer_integer_map',
									'01'	=> {
										'device_value'		=> 0xFF,
										'parameter_value'	=> 10,
										'from_device'		=> 1,
										'to_device'			=> 0,
									}
								}
							}
						}
					},
					'link'	=> {
						'peer_param'	=> 'actuator',
						'channel_param'	=> 'channel',
						'count'			=> 28,
						'address_start'	=> 0x357,
						'address_step'	=> 6,
						'channel'	=> {
							'operations'	=> 'none', 
							'hidden'		=> 1,
							'logical'		=> {
								'type'		=> 'int',
								'min'		=> 0,
								'max'		=> 255,
								'default'	=> 255,
							},
							'physical'	=> {
								'type'			=> 'int',
								'size'			=> 1,
								'interface'		=> 'eeprom',
								'address_id'	=> 0
							}
						},
						'actuator'	=> {
							'operations'	=> 'none', 
							'hidden'		=> 1,
							'logical'		=> {
								'type'		=> 'address',
							},
							'physical'		=> {
								'type'		=> 'array',
								'01'		=> {
									'type'		=>	'int',
									'size'		=>	4,
									'interface'	=>	'eeprom',
									'address_id'	=> 1
								},
								'02'	=> {
									'type'		=>	'int',
									'size'		=>	1,
									'interface'	=>	'eeprom',
									'address_id'	=> 5
								}
							}
						}
					},
					'values'	=> {
						'press_short'	=> {
							'operations'	=> 'event,write', 
							'control'		=> 'button.short',
							'logical'		=> {
								'type'		=> 'action',
							},
							'physical'		=> {
								'type'		=> 'int',
								'interface'	=> 'command',
								'value_id'	=> 'counter',
								'event'		=> {
									'frame'	=> 'key_event_short',
								},
								'set'		=> {
									'request'	=> 'key_sim_short',
								}
							},
							'conversion'	=> {
								'type'			=> 'action_key_counter',
								'sim_counter'	=> 'sim_counter',
								'counter_size'	=> 6
							}
						},
						'press_long'	=> {
							'operations'	=> 'event,write', 
							'control'		=> 'button.long',
							'logical'		=> {
								'type'		=> 'action',
							},
							'physical'		=> {
								'type'		=> 'int',
								'interface'	=> 'command',
								'value_id'	=> 'counter',
								'event'		=> {
									'frame'	=> 'key_event_long',
								},
								'set'		=> {
									'request'	=> 'key_sim_long',
								}
							},
							'conversion'	=> {
								'type'			=> 'action_key_counter',
								'sim_counter'	=> 'sim_counter',
								'counter_size'	=> 6
							}
						},
						'release_long'	=> {
							'operations'	=> 'event,write', 
							'control'		=> 'button.release',
							'logical'		=> {
								'type'		=> 'action',
							},
							'physical'		=> {
								'type'		=> 'int',
								'interface'	=> 'command',
								'value_id'	=> 'counter',
								'event'		=> {
									'frame'	=> 'key_event_longrelease',
								},
								'set'		=> {
									'request'	=> 'key_sim_long',
								}
							},
							'conversion'	=> {
								'type'			=> 'action_key_counter',
								'sim_counter'	=> 'sim_counter',
								'counter_size'	=> 6
							}
						},
						'install_test'	=> {
							'operations'	=> 'event', 
							'ui_flags'		=> 'internal',
							'logical'		=> {
								'type'		=> 'action',
							},
							'physical'		=> {
								'type'		=> 'int',
								'interface'	=> 'command',
								'value_id'	=> 'test_counter',
								'event'		=> {
									'frame'	=> 'key_event_short, key_event_long',
								}
							}
						}
					}
				}
			}
		}
	}
);

1;