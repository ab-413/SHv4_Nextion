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

NexText t5 = NexText(0, 11, "t5");  // Текст температура t2
NexText t6 = NexText(0, 12, "t6");  // Текст температура t1
NexText t7 = NexText(0, 13, "t7");  // Текст температура t3
NexText t8 = NexText(0, 14, "t8");  // Текст температура t4
NexText t14 = NexText(0, 20, "t14");  // Текст температура t5

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
// const uint16_t cont_node = 02;  // Контейнер узел

//----------------------------------   -------------------------------
NexTouch *nex_listen_list[] =
    {
        &bt0,
        &bt1,
        &bt2,
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
  t14.setText(t5str);

  j0.setValue(data.w1);

  bt0.setValue(data.d_pump);
  bt1.setValue(data.u_pump);
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
      digitalWrite(2, HIGH);
      delay(100);
      digitalWrite(2, LOW);
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
  }
  nexLoop(nex_listen_list);
}