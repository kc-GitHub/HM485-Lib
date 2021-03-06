package HM485::Devicefile;

our %definition = (
	'HBW_Sen_SC8'	=> {
		'version'		=> 12,
		'eep_size'	=> 1024,
		'supported_types' => 	{
			"HBW_Sen_SC8" => {
				"name" => "RS485 Homebrew Tastermodul (8fach)",
				"parameter" => {
					"0" => {
						"const_value" => 134,
						"size" => 1
					},
					"1" => {
						"const_value" => 0,
						"size" => 1
					},
					"2" => {
						"cond_op" => "GE",
						"const_value" => 0x0100,
						"size" => 2
					}
				},
				"priority" => 2
			}
		},
		'paramset' => {
			"enforce" => {
				"id" => "central_address",
				"value" => 1,
				"direct_link_deactivate" => {
					"value" => true
				}
			},
			"id" => "HBW_Sen_SC8_dev_master",
			"parameter" => {
					"hidden" => true,
					"id" => "central_address",
					"logical" => {
						"type" => "integer"
					},
					"physical" => {
						"address" => {
							"index" => 0x0002
						},
						"interface" => "eeprom",
						"size" => 4,
						"type" => "integer"
					},
				"direct_link_deactivate" => {
					"hidden" => true,
					"logical" => {
						"default" => false,
						"type" => "boolean"
					},
					"physical" => {
						"address" => {
							"index" => 0x0006
						},
						"interface" => "eeprom",
						"size" => 0.1,
						"type" => "integer"
					}
				},
				"logging_time" => {
					"conversion" => {
						"factor" => 10,
						"offset" => 0.0,
						"type" => "float_integer_scale"
					},
					"logical" => {
						"default" => 2.0,
						"max" => 25.5,
						"min" => 0.1,
						"type" => "float",
						"unit" => "s"
					},
					"physical" => {
						"address" => {
							"index" => 0x0001
						},
						"interface" => "eeprom",
						"size" => 1.0,
						"type" => "integer"
					}
				}
			},
			"type" => "master"
		},
		'frames'	=> {
			'level_set'	=> {
				'type'		=> 0x78,
				'direction'	=> 'to_device',
				'channel_field'	=> 10,
				'parameter'	=> {
						'type'	=> 'integer',
						'index'	=> 11.0,
						'size'	=> 1.0,
						'param' => "state"		
				},
			},
			'level_get'	=> {
				'type'		=> 0x53,
				'direction'		=> 'to_device',
				'channel_field'	=> 10,
			},
			'info_level'	=> {
				'type'		=> 0x69,
				'direction'		=> 'from_device',
				'event'		=> true,
				'channel_field'	=> 10,
				'parameter'	=> {
					"11.0" => {
						"param" => 'state',
						'type'	=> 'integer',
						'size'	=> 4.0
					},
					"15.4" => {
						"param" => 'state_flags',
						'type'	=> 'integer',
						'size'	=> 0.3
					},
				},
			},
			"key_event_short" => {
				"channel_field" => 10,
				"direction" => "from_device",
				"event" => true,
				"parameter" => {
					"12.0" => {
						"const_value" => 2,
						"size" => 0.2,
						"type" => "integer"
					},
					"12.2" => {
						"param" => "counter",
						"size" => 0.6,
						"type" => "integer"
					}
				},
				"type" => 0x4B
			},
			"key_event_long" => {
				"channel_field" => 10,
				"direction" => "from_device",
				"event" => true,
				"parameter" => {
					"12.0" => {
						"const_value" => 3,
						"size" => 0.2,
						"type" => "integer"
					},
					"12.2" => {
						"param" => "counter",
						"size" => 0.6,
						"type" => "integer"
					}
				},
				"type" => 0x4B
			},
			'key_event_longrelease'	=> {
				'type'		=> 0x4B,
				'direction'		=> 'from_device',
				'event'		=> true,
				'ch_field'	=> 10,
				'parameter'	=> {
					"12.0" => {
						"const_value" => 1,
						"size" => 0.2,
						"type" => "integer"
					},
					"12.2" => {
						"param" => "counter",
						"size" => 0.6,
						"type" => "integer"
					}
				}
			},
			"set_lock" => {
				"channel_field" => 11,
				"direction" => "to_device",
				"parameter" => {
					"index" => 12.0,
					"param" => "inhibit",
					"size" => 1.0,
					"type" => "integer"
				},
				"type" => 0x6C
			},
			"toggle_install_test" => {
				"channel_field" => 10,
				"direction" => "to_device",
				"parameter" => {
					"index" => 11.0,
					"param" => "toggle_flag",
					"size" => 1.0,
					"type" => "integer"
				},
				"type" => 0x78
			}
		},
		'channels'	=> {
			"maintenance" => {
				"class" => "maintenance",
				"count" => 1,
				"index" => 0,
				"paramset" => {
					"maint_ch_master" => {
						"type" => "master"
					},
					"maint_ch_values" => {
						"parameter" => {
							"config_pending" => {
								"logical" => {
									"type" => "boolean"
								},
								"operations" => "read,event",
								"physical" => {
									"interface" => "internal",
									"type" => "integer",
									"value_id" => "config_pending"
								},
								"ui_flags" => "service"
							},
							"sticky_unreach" => {
								"logical" => {
									"type" => "boolean"
								},
								"operations" => "read,write,event",
								"physical" => {
									"interface" => "internal",
									"type" => "integer",
									"value_id" => "sticky_unreach"
								},
								"ui_flags" => "service"
							},
							"unreach" => {
								"logical" => {
									"type" => "boolean"
								},
								"operations" => "read,event",
								"physical" => {
									"interface" => "internal",
									"type" => "integer",
									"value_id" => "unreach"
								},
								"ui_flags" => "service"
							}
						},
						"type" => "values"
					}
				},
				"ui_flags" => "internal"
			},
			'key'	=> {
				'index'	=> 1,
				'count'	=> 8,
				'physical_index_offset'	=> -1,
				"link_roles" => {
					"source" => {
						"name" => "switch"
					}
				},
				'paramset'	=> {
					'master'	=> {
						'address_start'	=> 0x07,
						'address_step'	=> 4,
						"parameter" => {
						'input_locked'	=> {
							'logical'	=> {
								'type'	=> 'boolean',
								'default'	=> false,
							},
							'physical'	=> {
								'type'			=> 'integer',
								'size'			=> 0.1,
								'interface'		=> 'eeprom',
									"address" => {
										"index" => 0.0
									},
							},
							'conversion'	=> {
								'type'		=> 'boolean_integer',
								'invert'	=> false
							}
						},
						'inverted'	=> {
							'logical'	=> {
								'type'	=> 'boolean',
								'default'	=> true,
							},
							'physical'	=> {
								'type'			=> 'integer',
								'size'			=> 0.1,
								'interface'		=> 'eeprom',
								"address" => {
									"index" => 1.0
								},
							},
							'conversion'	=> {
								'type'		=> 'boolean_integer',
								'invert'	=> false
							}
						},
						'pullup'	=> {
							'logical'	=> {
								'type'	=> 'boolean',
								'default'	=> true,
							},
							'physical'	=> {
								'type'			=> 'integer',
								'size'			=> 0.1,
								'interface'		=> 'eeprom',
								"address" => {
									"index" => 2.0
								},
							},
							'conversion'	=> {
								'type'		=> 'boolean_integer',
								'invert'	=> false
							}
						},
						'long_press_time'	=> {
							"conversion" => {
								"1" => {
									"factor" => 10,
									"type" => "float_integer_scale"
								},
								"2" => {
									"type" => "integer_integer_map",
									"value_map" => {
										"device_value" => 0xFF,
										"from_device" => true,
										"parameter_value" => 10,
										"to_device" => false
									}
								}
							},
							"logical" => {
								"default" => 1.0,
								"max" => 5.0,
								"min" => 0.4,
								"type" => "float",
								"unit" => "s"
							},
							"physical" => {
								"address" => {
									"index" => 3.0
								},
								"interface" => "eeprom",
								"size" => 1.0,
								"type" => "integer"
							}
						}
						}
					},
					'link'	=> {
						"type" => "link",
						'peer_param'	=> 'actuator',
						'channel_param'	=> 'channel',
						'count'			=> 28,
						'address_start'	=> 0x357,
						'address_step'	=> 6,
						"parameter" => {
						'channel'	=> {
							'operations'	=> 'none', 
							'hidden'		=> true,
							'logical'		=> {
								'type'		=> 'integer',
								'min'		=> 0,
								'max'		=> 255,
								'default'	=> 255,
							},
							'physical'	=> {
								'type'			=> 'integer',
								'size'			=> 1.0,
								'interface'		=> 'eeprom',
								"address" => {
										"index" => 0
								},
							}
						},
						'actuator'	=> {
							'operations'	=> 'none', 
							'hidden'		=> true,
							'logical'		=> {
								'type'		=> 'address',
							},
							"physical" => [
								{
									"address" => {
										"index" => 1
									},
									"interface" => "eeprom",
									"size" => 4.0,
									"type" => "integer"
								},
								{
									"address" => {
										"index" => 5
									},
									"interface" => "eeprom",
									"size" => 1.0,
									"type" => "integer"
								}
							]
						}
						}
					},
					'values'	=> {
					"type" => "values",
					"parameter" => {
						'press_short'	=> {
							'operations'	=> 'event,write', 
							'control'		=> 'button.short',
							'logical'		=> {
								'type'		=> 'action',
							},
							'physical'		=> {
								'type'		=> 'integer',
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
								'type'		=> 'integer',
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
								'type'		=> 'integer',
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
								'type'		=> 'integer',
								'interface'	=> 'command',
								'value_id'	=> 'test_counter',
									"event" => {
										"1" => {
											"frame" => "key_event_short"
										},
										"2" => {
											"frame" => "key_event_long"
										}
									},
							}
						}
					}
					}
				}
			}
		}
	}
);	
