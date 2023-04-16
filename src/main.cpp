#include <SPI.h>
#include <RF24Network.h>
#include <RF24.h>
#include "Nextion.h"

#define RF_CE 9     // nRF24l01 CE pin
#define RF_CSN 10   // nRF24l01 CS0 pin

//----------------------------------Создание объектов-------------------------------
RF24 radio(RF_CE, RF_CSN);
RF24Network network(radio);

//----------------------------------Создание объектов Nextion-------------------------------
NexDSButton bt0 = NexDSButton(0, 4, "bt0");  // Кнопка насос внизу
NexDSButton bt1 = NexDSButton(0, 5, "bt1");  // Кнопка насос давление
NexDSButton bt2 = NexDSButton(0, 10, "bt2");  // Кнопка резерв
NexButton bt3 = NexButton(1, 5, "bt3");

NexText t5 = NexText(0, 11, "v1");  // Текст температура 2 этаж
NexText t6 = NexText(0, 12, "v2");  // Текст температура 1 этаж
NexText t7 = NexText(0, 13, "v3");  // Текст температура Гараж
NexText t8 = NexText(0, 14, "v4");  // Текст температура Улица

NexText nex_power_mode = NexText(1, 11, "v7");  
NexText nex_pump = NexText(1, 12, "v8");  
NexText nex_error = NexText(1, 13, "v9");  
NexText nex_run = NexText(1, 14, "v10");  
NexText nex_targ_air = NexText(1, 15, "v11");
NexText nex_targ_water = NexText(1, 16, "v12");
NexText nex_hyst_air = NexText(1, 17, "v13");
NexText nex_hyst_water = NexText(1, 20, "v14");

NexText nex_cur_air = NexText(0, 21, "v5"); 
NexText nex_cur_water = NexText(0, 23, "v6");

NexProgressBar j0 = NexProgressBar(0, 1, "j0"); // Прогрессбар уровень воды

//----------------------------------Переменные-------------------------------
boolean PUMP_0_STATE = false; // Состояние реле 0
boolean OLD_PUMP_0_STATE = false;
boolean PUMP_1_STATE = false; // Состояние реле 1
boolean OLD_PUMP_1_STATE = false;
boolean need_send = false;

//----------------------------------Адреса узлов с сети-------------------------------
const uint16_t base_node = 00;  // Базовый узел
const uint16_t this_node = 01;  // Этот узел
const uint16_t boiler_node = 02;  // Котел узел

//----------------------------------   -------------------------------
NexTouch *nex_listen_list[] =
    {
        &bt0,
        &bt1,
        &bt2,
        &bt3,
        NULL};

//----------------------------------Структура с данными-------------------------------
struct DATA_STRUCTURE
{
  uint8_t w1;
  float t1;
  float t2;
  float t3;
  float t4;
  float t5;
  uint8_t humidity;
  boolean d_pump;
  boolean u_pump;
} __attribute__((packed));  // Упаковка данных поплотнее=)
DATA_STRUCTURE data;

struct CURRENT_DATA_STRUCTURE
{
  uint8_t pwr_mode=0; // 0-off...3-full  
  bool pump_state=0;
  bool error=0;
  bool run=0;
  float water_current_temperature = 0;
  float air_current_temperature = 0;
} __attribute__((packed));
CURRENT_DATA_STRUCTURE current_boiler;

struct TARGET_DATA_STRUCTURE
{
  float water_target_temperature = 60.0;
  float water_hysteresis = 5.0;
  float air_target_tempertature = 25.0;
  float air_hysteresis = 5.0;
} __attribute__((packed));
TARGET_DATA_STRUCTURE target_boiler;

//----------------------------------Данные с Nextion-------------------------------
void bt0PopCallback(void *ptr)  // 
{
  uint32_t dual_state0;  

  bt0.getValue(&dual_state0);
  if (dual_state0)
  {
    PUMP_0_STATE = 1;
    need_send = true;   // Флаг отправки
  }
  else
  {
    PUMP_0_STATE = 0;
    need_send = true;
  }  
}

void bt1PopCallback(void *ptr)
{
  uint32_t dual_state1;

  bt1.getValue(&dual_state1);
  if (dual_state1)
  {
    PUMP_1_STATE = 1;
    need_send = true;
  }
  else
  {
    PUMP_1_STATE = 0;
    need_send = true;
  }  
}

void bt2PopCallback(void *ptr)
{
  uint32_t dual_state2;

  bt2.getValue(&dual_state2);
  // if(dual_state2){
  //   digitalWrite(13,HIGH);
  // }
  // else{
  //   digitalWrite(13,LOW);
  // }
}

// void bt3PopCallback(void *ptr)
// {
//   RF24NetworkHeader transmitter(boiler_node, 'N');
//   network.write(transmitter, &target_boiler, sizeof(target_boiler));   
// }

void sendDatatoNext()   // Отправка данных на дисплей
{
  char t1str[8];
  dtostrf(data.t1, 5, 1, t1str);
  char t2str[8];
  dtostrf(data.t2, 5, 1, t2str);
  char t3str[8];
  dtostrf(data.t3, 5, 1, t3str);
  char t4str[8];
  dtostrf(data.t4, 5, 1, t4str);
  char t5str[8];
  dtostrf(data.t5, 5, 1, t5str);

  t5.setText(t1str);
  t6.setText(t2str);
  t7.setText(t3str);
  t8.setText(t4str);
  
  j0.setValue(data.w1);

  bt0.setValue(data.d_pump);
  bt1.setValue(data.u_pump);
}

void sendDatatoNextBoiler()   // Отправка данных на дисплей
{
  char nex_power_mode_str[8];
  dtostrf(current_boiler.pwr_mode, 2, 0, nex_power_mode_str);
  char nex_pump_str[8];
  dtostrf(current_boiler.pump_state, 2, 0, nex_pump_str);
  char nex_error_str[8];
  dtostrf(current_boiler.error, 2, 0, nex_error_str);
  char nex_run_str[8];
  dtostrf(current_boiler.run, 2, 0, nex_run_str);
  char nex_targ_air_str[8];
  dtostrf(target_boiler.air_target_tempertature, 5, 1, nex_targ_air_str);
  char nex_targ_water_str[8];
  dtostrf(target_boiler.water_target_temperature, 5, 1, nex_targ_water_str);
  char nex_hyst_air_str[8];
  dtostrf(target_boiler.air_hysteresis, 5, 1, nex_hyst_air_str);
  char nex_hyst_water_str[8];
  dtostrf(target_boiler.water_hysteresis, 5, 1, nex_hyst_water_str);
  char nex_cur_air_str[8];
  dtostrf(current_boiler.air_current_temperature, 5, 1, nex_cur_air_str);
  char nex_cur_water_str[8];
  dtostrf(current_boiler.water_current_temperature, 5, 1, nex_cur_water_str);

  nex_power_mode.setText(nex_power_mode_str);
  nex_pump.setText(nex_pump_str);
  nex_error.setText(nex_error_str);
  nex_run.setText(nex_run_str);
  nex_targ_air.setText(nex_targ_air_str);
  nex_targ_water.setText(nex_targ_water_str);
  nex_hyst_air.setText(nex_hyst_air_str);
  nex_hyst_water.setText(nex_hyst_water_str);
  nex_cur_air.setText(nex_cur_air_str);
  nex_cur_water.setText(nex_cur_water_str);
}

//////////////////////////////////////////////////////////

void setup(void)
{
  /* Инициализация nRF24 */
  SPI.begin();
  radio.begin();
  radio.setChannel(70);
  network.begin(this_node);  

  /* Инициализация дисплея */
  nexInit();
  bt0.attachPop(bt0PopCallback, &bt0);
  bt1.attachPop(bt1PopCallback, &bt1);
  bt2.attachPop(bt2PopCallback, &bt2);
  // bt3.attachPop(bt3PopCallback, &bt3);
  pinMode(2, OUTPUT);
}

//////////////////////////////////////////////////////////

void loop(void)
{
  network.update();

  //----------------------------------Отправка по флагу-------------------------------
  if (need_send){
    data.u_pump = PUMP_1_STATE;
    data.d_pump = PUMP_0_STATE;
    RF24NetworkHeader transmitter(base_node, 'T');
    boolean ok = network.write(transmitter, &data, sizeof(data));    
    if (ok) {
      digitalWrite(13, HIGH);
      delay(100);
      digitalWrite(13, LOW);
      need_send = false;
      OLD_PUMP_0_STATE = PUMP_0_STATE;       
      OLD_PUMP_1_STATE = PUMP_1_STATE;
    }    
  }    

  //----------------------------------Прием данных-------------------------------

  while (network.available())
  { 
    RF24NetworkHeader receiver;
    network.peek(receiver);
    if (receiver.type == 'T')
    {
      network.read(receiver, &data, sizeof(data)); // читаем данные и указываем сколько байт читать

      PUMP_0_STATE = data.d_pump;
      OLD_PUMP_0_STATE = PUMP_0_STATE;

      PUMP_1_STATE = data.u_pump;
      OLD_PUMP_1_STATE = PUMP_1_STATE;

      sendDatatoNext();
    }
    if (receiver.type == 'N')
    {
      network.read(receiver, &current_boiler, sizeof(current_boiler)); // читаем данные и указываем сколько байт читать

      sendDatatoNextBoiler();
    }
  }
  nexLoop(nex_listen_list);
}