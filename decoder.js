function decodeUplink(input) {
  var data={};
      data.external_temperature= ((input.bytes[0]<<8) + (input.bytes[1]))/100,
      data.humidity= ((input.bytes[2]<<8) + (input.bytes[3]))/100,
      data.soil_moisture_level= ((input.bytes[4]<<8) + (input.bytes[5])),
      data.internal_temperature= ((input.bytes[6]<<8) + (input.bytes[7]))/100,
      data.pressure=(((input.bytes[8]<<24) + (input.bytes[9])<<16) + (input.bytes[10]<<8) + (input.bytes[11]))/100,
      data.voltage= ((input.bytes[12]<<8) + (input.bytes[13]))/1000
  return {
data: data
  };
}
