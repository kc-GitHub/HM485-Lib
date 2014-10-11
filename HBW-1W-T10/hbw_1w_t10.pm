package HM485::Devicefile;

our %definition = (
	'HBW_1W_T10'	=>{
		'version'		=> 12,
		'eeprom-size'	=> 1024,
		'models'	=> {
			'HBW_1W_T10'	=> {
				'name'	=> 'Homebrew wired Temp-Sensor Modul',
				'type'	=> 129,
			},
		},
		'params' => {
			'master'	=> {
				'central_address'	=> {
					'hidden'		=> 1,
					'enforce'		=> 0x00000001,
					'logical'		=> {
						'type'		=> 'int',
					},
					'physical'			=> {
						'type'			=> 'int',
						'size'			=> 4,
						'interface'		=> 'eeprom',
						'address_id'	=> 0x0002
					}
				}
			}
		},
		'frames'	=> {
			'level_get'	=> {
				'type'		=> 0x53,
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
						'size'	=> 2
					}
				}
			},
			'info_frequency'	=> {
				'type'		=> 0x69,
				'dir'		=> 'from_device',
				'event'		=> 1,
				'ch_field'	=> 10,
				'params'	=> {
					'state'		=> {
						'type'	=> 'int',
						'id'	=> 11.0,
						'size'	=> 3
					},
				}
			}
		},
		'channels'	=> {
			'maintenance'	=> {
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
			'Temperature'	=> {
				'id'	=> 1,
				'count'	=> 10,
				'physical_id_offset'	=> -1,
				'special_param'	=> {
					'behaviour'	=> {
						'logical'	=> {
							'type'	=> 'int',
						},
						'physical'	=> {
							'type'			=> 'int',
							'size'			=> 0.1,
							'interface'		=> 'eeprom',
							'address_id'	=> 8.0,
							'address_step'	=> 0.1 
						}
					}
				},
				'params'	=> {
					'master'	=> {
						'address_start'	=> 0x07,
						'address_step'	=> 6,
						'send_delta_temp'	=> {
							'logical'	=> {
								'type'		=> 'integer',
								'min' 		=> 0.1,
								'max'	 	=> 25,
								'default'	=> 0.5,
							},
							'physical'	=> {
								'type'			=> 'int',
								'size'			=> 1,
								'interface'		=> 'eeprom',
								'address_id'	=> 1.0
							}
						},
						'send_min_interval'	=> {
							'logical'	=> {
								'type'		=> 'float',
								'min' 		=> 5,
								'max'	 	=> 3600,
								'default'	=> 10,
								'unit'		=> 's',
							},
							'physical'	=> {
								'type'			=> 'int',
								'size'			=> 2,
								'interface'		=> 'eeprom',
								'endian'		=> 'little',
								'address_id'	=> 3.0
							}
						},
						'send_max_interval'	=> {
							'logical'	=> {
								'type'		=> 'float',
								'min' 		=> 5,
								'max'	 	=> 3600,
								'default'	=> 150,
								'unit'		=> 's',
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
					'values'	=> {
						'temperature' => {
							'operations'	=> 'read,event', 
							'logical'		=> {
								'type'		=> 'float',
								'min'		=> -273.15,
								'max'		=> 300,
							},
							'physical'		=> {
								'type'		=> 'int',
								'interface'	=> 'command',
								'value_id'	=> 'state',
								'get'		=> {
									'request'	=> 'level_get',
									'response'	=> 'info_level'
								},
								'event'		=> {
									'frame'	=> 'info_level'
								},
							},
							'conversion'	=> {
								'type'		=> 'float_integer_scale',
								'factor'	=> 100
							}
						}
					}
				},
				'subconfig'	=> {
					'master'	=> {
					},
					'values'	=> {
						'state'	=> {
							'operations'	=> 'read,event',
							'logical'		=> {
								'type'		=> 'boolean',
								'default'	=> 0,
							},
							'physical'		=> {
								'type'		=> 'int',
								'interface'	=> 'command',
								'value_id'	=> 'state',
								'get'		=> {
									'request'	=> 'level_get',
									'response'	=> 'info_level'
								},
								'event'		=> {
									'frame'	=> 'info_level'
								}
							},
							'conversion'	=> {
								'type'		=> 'boolean_integer',
								'threshold'	=> 1,
								'false'		=> 0,
								'true'		=> 1023
							}
						}
					}
				}
			}
		}
	}
);

1;