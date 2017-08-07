
#include <Wire.h>
#include <LiquidCrystal.h>
#include <Adafruit_MotorShield.h>
  
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

//Define valores usados pelos botões
int lcd_key     = 0;
int adc_key_in  = 0;
#define btnRIGHT  5
#define btnUP     4
#define btnDOWN   3
#define btnLEFT   2
#define btnSELECT 1
#define btnNONE   0

//Função para ler os botões
int read_LCD_buttons()
{
	adc_key_in = analogRead(0);      // read the value from the sensor 
	// my buttons when read are centered at these valies: 0, 144, 329, 504, 741
	// we add approx 50 to those values and check to see if we are close
	if (adc_key_in > 1000) return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result
	// For V1.1 us this threshold
	if (adc_key_in < 50)   return btnRIGHT;  
	if (adc_key_in < 250)  return btnUP; 
	if (adc_key_in < 450)  return btnDOWN; 
	if (adc_key_in < 650)  return btnLEFT; 
	if (adc_key_in < 850)  return btnSELECT;  

 	return btnNONE;  // when all others fail, return this...
}

//Criando o objeto motor shield com o endereco default I2C
Adafruit_MotorShield AFMS = Adafruit_MotorShield();

//Selecionando a porta de cada motor
Adafruit_DCMotor *leftMotor = AFMS.getMotor(3);
Adafruit_DCMotor *rigthMotor = AFMS.getMotor(4);

// Definicao de pinos
#define leftEncoder 27
#define rigthEncoder 29
#define ldr A12

int leftEncState, rightEncState, getOut, getOutLp, oldLeftEncState, oldRightEncState, rightEncCount, leftEncCount, ldrInputValue, minLdrValue, ligthLeftEncoder, ligthRightEncoder, getOutLine, getOutAngle;
double angRightMotor, angLeftMotor, ang; // Angulo em graus
double distRightMotor = 0, distLeftMotor = 0;

//Pinos conectados aos sensores óptico-reflexivos
const int rightSensor = A9;
const int leftSensor = A8;
//Valores de leitura dos sensores
int rightValue = 0;
int leftValue = 0;
//Variáveis para as superfícies
int whiteSurfaceR = 0;
int blackSurfaceR = 0;
int whiteSurfaceL = 0;
int blackSurfaceL = 0;
int setPointR = 0;
int setPointL = 0;
//Ganhos Line-Following
int Kp = 0;
int Ki = 0;
int Kd = 0;
int correctionR = 0;
int correctionL = 0;

//Intervalo de mudança do Display
const long intervalo_display = 400;
//Variáveis para armazenar tempo
unsigned long tempo_anterior = 0, tempo_atual = 0;

//Comprimento das Figuras
int dist = 0;


void setup(){

	//Display com 16 colunas e 2 linhas
	lcd.begin(16, 2);

	//Inicia o objeto de motor com a frequencia default = 1.6KHz
	AFMS.begin();

	//Atribuindo os valores iniciais
	leftEncState = LOW;
	rightEncState = LOW;
	oldLeftEncState = LOW;
	oldRightEncState = LOW;
	leftEncCount = 0;
	rightEncCount = 0;
	getOut = 0;
	getOutLp = 0;
	getOutLine = 0;
  	getOutAngle = 0;
	angRightMotor = 0; 
	angLeftMotor = 0;
	ldrInputValue = 9999; // Variavel para armazenar a leitura atual do sensor ldr
	minLdrValue = 9999;
	ligthLeftEncoder = 0; // Pontos do encoder que ocorreram a iluminacao maxima
	ligthRightEncoder = 0;

	pinMode(leftEncoder,INPUT);
  	pinMode(rigthEncoder,INPUT);
}
 

void loop() 
{

	int i, j, tamanho;

//Tela Inicial
	lcd.clear();
	lcd.setCursor(0,0);
	lcd.print("Mandela TP3");
	lcd.setCursor(0,1);
	lcd.print("Iniciar (SELECT)");           
	while(lcd_key != 1){
		lcd_key = read_LCD_buttons();  //Lê os botões
	}
	delay(300);
	lcd_key = 0;

//Selecionar tarefas
	lcd.clear();
	lcd.setCursor(0,0);
	lcd.print("Escolha a tarefa");
	char texto1[] = "1-Localizacao(LEFT) 2-Odometria e Ctrl(DOWN) 3-Navegacao(UP)"; //60 caracteres
	tamanho = 60;
	for(i = 0; i < 16; i++){
		lcd.setCursor(i,1);
		lcd.print(texto1[i]);
	}
	
	i = 16;
	tempo_anterior = millis();
	while((lcd_key != 2)&&(lcd_key != 3)&&(lcd_key != 4)){		
		lcd_key = read_LCD_buttons();
		tempo_atual = millis();
		if(tempo_atual - tempo_anterior >= intervalo_display){
			tempo_anterior = tempo_atual;
			if(i < tamanho){
				for(j = 0; j < 16; j++){
					lcd.setCursor(j,1);
					lcd.print(texto1[i-15+j]);
				}
				i++;
			}
			else{
				for(j = 0; j < 16; j++){
					lcd.setCursor(j,1);
					lcd.print(texto1[j]);
				}
				i = 16;
			}
		}
	}
	
	//Localização
	if(lcd_key == 2){
		delay(300);
		lcd_key = 0;
		lcd.clear();
		lcd.setCursor(0,0);
		lcd.print("Executando...");
		lcd.setCursor(0,1);
		lcd.print("Sair (SELECT)");

		// Atribuindo os valores iniciais
		leftEncState = LOW;
		rightEncState = LOW;
		oldLeftEncState = LOW;
		oldRightEncState = LOW;
		leftEncCount = 0;
		rightEncCount = 0;
		getOut = 0;
		getOutLp = 0;
		angRightMotor = 0; 
		angLeftMotor = 0;
		ldrInputValue = 9999; // Variavel para armazenar a leitura atual do sensor ldr
		minLdrValue = 9999;
		ligthLeftEncoder = 0; // Pontos do encoder que ocorreram a iluminacao maxima
		ligthRightEncoder = 0;

		//Quantas voltas o encoder deve dar para atingir o angulo necessario
		ang = 360;
		angLeftMotor = 0.0611*ang;
		angRightMotor = 0.0667*ang;

	  	// Gira 360 e encontra o ponto de luz mais proximo (menor leitura ldr)
	  	while (getOut != 1){
		 
		 // Condicao de parada
		 // Verifica se os motores atingiram o num de voltas necessaria para o ang requisitado
		 if((leftEncCount>=angLeftMotor)&&(rightEncCount>=angRightMotor)){
			getOut = 1;
		 }

		 // Caso seja terminado
		 if (getOut == 1){
			leftMotor->run(RELEASE);
			rigthMotor->run(RELEASE);

			delay(2000);

			// Volta para a luz
		
			leftEncState = LOW;
			rightEncState = LOW;
			oldLeftEncState = LOW;
			oldRightEncState = LOW;
			leftEncCount = 0;
			rightEncCount = 0;
		
			while (getOutLp != 1){
			  
			  // Condicao de parada
			  // Verifica se os motores atingiram o num de voltas necessaria para o ang requisitado
			  if((leftEncCount>=ligthLeftEncoder)&&(rightEncCount>=ligthRightEncoder)){
				 getOutLp = 1;
			  }
		
			  if (getOutLp == 1){
				 leftMotor->run(RELEASE);
				 rigthMotor->run(RELEASE);
				 break;
			  }
			  
			  // Gira o robo para a direita
			  leftMotor->run(FORWARD);
			  leftMotor->setSpeed(120);
			  rigthMotor->run(BACKWARD);
			  rigthMotor->setSpeed(142);
		
			  // Faz a leitura dos encoderes
			  leftEncState = digitalRead(leftEncoder);
			  rightEncState = digitalRead(rigthEncoder);
		
			  // Verifica borda de descida do encoder esquerdo
			  if (leftEncState == HIGH){
				 if (oldLeftEncState == LOW){
				   leftEncCount++;
				 }
			  } else{ //Verifica borda de subida
				 if (oldLeftEncState == HIGH){
				   leftEncCount++;
				 }
			  }
		
			  // Verifica borda de descida do encoder direito
			  if (rightEncState == HIGH){
				 if (oldRightEncState == LOW){
				   rightEncCount++;
				 }
			  } else { //Verifica borda de subida
				 if (oldRightEncState == HIGH){
				   rightEncCount++;
				 }
			  }
				   
			  oldRightEncState = rightEncState;
			  oldLeftEncState = leftEncState;
			  
			}
	  
		
			break;
		 }

		 // Gira o robo 360 graus para a direita
		 leftMotor->run(FORWARD);
		 leftMotor->setSpeed(120);
		 rigthMotor->run(BACKWARD);
		 rigthMotor->setSpeed(142);

		 // Faz a leitura dos encoderes
		 leftEncState = digitalRead(leftEncoder);
		 rightEncState = digitalRead(rigthEncoder);

		 // Verifica borda de descida do encoder esquerdo
		 if (leftEncState == HIGH){
			if (oldLeftEncState == LOW){
			  leftEncCount++;
			}
		 } else{ //Verifica borda de subida
			if (oldLeftEncState == HIGH){
			  leftEncCount++;
			}
		 }

		 // Verifica borda de descida do encoder direito
		 if (rightEncState == HIGH){
			if (oldRightEncState == LOW){
			  rightEncCount++;
			}
		 } else { //Verifica borda de subida
			if (oldRightEncState == HIGH){
			  rightEncCount++;
			}
		 }
		 
		 // Faz a leitura do sensor ldr
		 ldrInputValue = analogRead(ldr);

		 // Verifica se o ponto luminoso eh maior
		 // O ponto luminoso ocorre qnd o ldr faz uma leitura pequena
		 if (ldrInputValue < minLdrValue){
			minLdrValue = ldrInputValue;
			ligthLeftEncoder = leftEncCount;
			ligthRightEncoder = rightEncCount;
		 }
			  
		 oldRightEncState = rightEncState;
		 oldLeftEncState = leftEncState;
		 
	  }
	
	}

	//Odometria e controle
	if(lcd_key == 3){
		delay(300);
		lcd_key = 0;
		lcd.clear();
		lcd.setCursor(0,0);
		lcd.print("Selecione:");
		char texto2[] = "1-Linha(LEFT) 2-Triangulo(DOWN) 3-Quadrado(UP) 4-Sair(SELECT)"; //61 caracteres
		tamanho = 61;
		for(i = 0; i < 16; i++){
			lcd.setCursor(i,1);
			lcd.print(texto2[i]);
		}
	
		i = 16;
		tempo_anterior = millis();
		while(1){		
			lcd_key = read_LCD_buttons();
			tempo_atual = millis();
			if(tempo_atual - tempo_anterior >= intervalo_display){
				tempo_anterior = tempo_atual;
				if(i < tamanho){
					for(j = 0; j < 16; j++){
						lcd.setCursor(j,1);
						lcd.print(texto2[i-15+j]);
					}
					i++;
				}
				else{
					for(j = 0; j < 16; j++){
						lcd.setCursor(j,1);
						lcd.print(texto2[j]);
					}
					i = 16;
				}
			}
			
			if(lcd_key == 1){
				delay(300);
				lcd_key = 0;
				break;
			}
			
			if(lcd_key == 2){
				delay(300);
				dist = 0;
				lcd_key = 0;
				lcd.clear();
				lcd.setCursor(0,0);
				lcd.print("Tamanho Linha:");
				lcd.setCursor(3,1);
				lcd.print("cm");
				lcd.setCursor(0,1);
				lcd.print(dist);
				lcd.print(" ");
				
				while(1){
					lcd_key = read_LCD_buttons();

					if(lcd_key == 4){
						delay(300);
						lcd_key = 0;
						if(dist <= 45){
							dist += 5;
						}
						lcd.setCursor(0,1);
						lcd.print(dist);
						lcd.print(" ");
					}
					if(lcd_key == 3){
						delay(300);
						lcd_key = 0;
						if(dist >= 5){
							dist -= 5;
						}
						lcd.setCursor(0,1);
						lcd.print(dist);
						lcd.print(" ");
					}

					if(lcd_key == 1){
						delay(300);
						lcd_key = 0;
						lcd.clear();
						lcd.setCursor(0,0);
						lcd.print("Executando...");
						lcd.setCursor(0,1);
						lcd.print("Linha");

						leftEncState = LOW;
						rightEncState = LOW;
						oldLeftEncState = LOW;
						oldRightEncState = LOW;
						leftEncCount = 0;
						rightEncCount = 0;
						getOutLine = 0;
						getOutAngle = 0;

						// Quantas voltas o encoder deve dar para atingir a distancia necessaria
						distRightMotor = 0.48*dist;
						distLeftMotor = 0.48*dist;

						//Movimenta no percurso linha considerando dist						
						linePath();

						lcd.clear();
						lcd.setCursor(0,0);
						lcd.print("Selecione:");
						break;
					}
				}
			}

			if(lcd_key == 3){
				delay(300);
				dist = 0;
				lcd_key = 0;
				lcd.clear();
				lcd.setCursor(0,0);
				lcd.print("Lado Triangulo:");
				lcd.setCursor(3,1);
				lcd.print("cm");
				lcd.setCursor(0,1);
				lcd.print(dist);
				lcd.print(" ");
				
				while(1){
					lcd_key = read_LCD_buttons();

					if(lcd_key == 4){
						delay(300);
						lcd_key = 0;
						if(dist <= 45){
							dist += 5;
						}
						lcd.setCursor(0,1);
						lcd.print(dist);
						lcd.print(" ");
					}
					if(lcd_key == 3){
						delay(300);
						lcd_key = 0;
						if(dist >= 5){
							dist -= 5;
						}
						lcd.setCursor(0,1);
						lcd.print(dist);
						lcd.print(" ");
					}

					if(lcd_key == 1){
						delay(300);
						lcd_key = 0;
						lcd.clear();
						lcd.setCursor(0,0);
						lcd.print("Executando...");
						lcd.setCursor(0,1);
						lcd.print("Triangulo");

						leftEncState = LOW;
						rightEncState = LOW;
						oldLeftEncState = LOW;
						oldRightEncState = LOW;
						leftEncCount = 0;
						rightEncCount = 0;
						getOutLine = 0;
						getOutAngle = 0;

						// Quantas voltas o encoder deve dar para atingir a distancia necessaria
						distRightMotor = 0.48*dist;
						distLeftMotor = 0.48*dist;

						//Movimenta no percurso linha considerando dist		
						ang = 120;				
						trianglePath();

						lcd.clear();
						lcd.setCursor(0,0);
						lcd.print("Selecione:");
						break;
					}
				}
			}

			if(lcd_key == 4){
				delay(300);
				dist = 0;
				lcd_key = 0;
				lcd.clear();
				lcd.setCursor(0,0);
				lcd.print("Lado Quadrado:");
				lcd.setCursor(3,1);
				lcd.print("cm");
				lcd.setCursor(0,1);
				lcd.print(dist);
				lcd.print(" ");
				
				while(1){
					lcd_key = read_LCD_buttons();

					if(lcd_key == 4){
						delay(300);
						lcd_key = 0;
						if(dist <= 45){
							dist += 5;
						}
						lcd.setCursor(0,1);
						lcd.print(dist);
						lcd.print(" ");
					}
					if(lcd_key == 3){
						delay(300);
						lcd_key = 0;
						if(dist >= 5){
							dist -= 5;
						}
						lcd.setCursor(0,1);
						lcd.print(dist);
						lcd.print(" ");
					}

					if(lcd_key == 1){
						delay(300);
						lcd_key = 0;
						lcd.clear();
						lcd.setCursor(0,0);
						lcd.print("Executando...");
						lcd.setCursor(0,1);
						lcd.print("Quadrado");
						
						leftEncState = LOW;
						rightEncState = LOW;
						oldLeftEncState = LOW;
						oldRightEncState = LOW;
						leftEncCount = 0;
						rightEncCount = 0;
						getOutLine = 0;
						getOutAngle = 0;

						// Quantas voltas o encoder deve dar para atingir a distancia necessaria
						distRightMotor = 0.48*dist;
						distLeftMotor = 0.48*dist;

						//Movimenta no percurso quadrado considerando dist		
						ang = 100;				
						squarePath();

						lcd.clear();
						lcd.setCursor(0,0);
						lcd.print("Selecione:");
						break;
					}
				}
			}

		}
	}

	//Navegação (line-following)
	if(lcd_key == 4){
		delay(300);
		lcd_key = 0;
		lcd.clear();
		lcd.setCursor(0,0);
		lcd.print("Navegacao:");
		char texto3[] = "1-Calibracao(LEFT) 2-Line-Following(DOWN) 3-Sair(SELECT)"; //56 caracteres
		tamanho = 56;
		for(i = 0; i < 16; i++){
			lcd.setCursor(i,1);
			lcd.print(texto3[i]);
		}
	
		i = 16;
		tempo_anterior = millis();
		while(1){		
			lcd_key = read_LCD_buttons();
			tempo_atual = millis();
			if(tempo_atual - tempo_anterior >= intervalo_display){
				tempo_anterior = tempo_atual;
				if(i < tamanho){
					for(j = 0; j < 16; j++){
						lcd.setCursor(j,1);
						lcd.print(texto3[i-15+j]);
					}
					i++;
				}
				else{
					for(j = 0; j < 16; j++){
						lcd.setCursor(j,1);
						lcd.print(texto3[j]);
					}
					i = 16;
				}
			}
		
			if(lcd_key == 1){
				delay(300);
				lcd_key = 0;
				break;
			}
		
			if(lcd_key == 2){
				delay(300);
				lcd_key = 0;
				//Rotina de Calibração
				LightSensorsCalibrate();
			}

			if(lcd_key == 3){
				delay(300);
				lcd_key = 0;
				
				//Atribui ao Setpoint de cada sensor um valor médio
				setPointR = (blackSurfaceR - whiteSurfaceR)/2 + whiteSurfaceR;
				setPointL = (blackSurfaceL - whiteSurfaceL)/2 + whiteSurfaceL;

				lcd.clear();
				lcd.setCursor(0,0);
				lcd.print("Kp = ");
				
				Kp = 1;

				leftMotor->run(FORWARD);
				leftMotor->setSpeed(60);
				rigthMotor->run(FORWARD);
				rigthMotor->setSpeed(72);
				
				while(1){
					lcd.setCursor(5,0);
					lcd.print(Kp);
				
					//Leitura dos sensores					
					rightValue = analogRead(rightSensor);
					leftValue = analogRead(leftSensor);
					
					//Erro = Setpoint - leitura
					correctionR = Kp * (setPointR - rightValue);
					correctionL = Kp * (setPointL - leftValue);

					lcd.setCursor(0,1);
					lcd.print(correctionL);
					lcd.print(" / ");
					lcd.print(correctionR);
					
					//Move para frente
					if(rightValue < setPointR && leftValue < setPointL){
						leftMotor->run(FORWARD);
						leftMotor->setSpeed(60);
						rigthMotor->run(FORWARD);
						rigthMotor->setSpeed(72);
					}
					//Move para direita
					else if(rightValue > setPointR && leftValue < setPointL){
						leftMotor->run(FORWARD);
						if(60 + correctionR <= 255){
							leftMotor->setSpeed(60 + correctionR);
						}
						else{
							leftMotor->setSpeed(255);
						}
						rigthMotor->run(BACKWARD);
						rigthMotor->setSpeed(72);
					}
					//Move para esquerda
					else if(rightValue < setPointR && leftValue > setPointL){
						rigthMotor->run(FORWARD);
						if(72 + correctionL <= 255){
							rigthMotor->setSpeed(72 + correctionL);
						}
						else{
							rigthMotor->setSpeed(255);
						}
						leftMotor->run(BACKWARD);
						leftMotor->setSpeed(60);
					}
					//Move também para esquerda, caso ambos sobre a linha preta
					else if(rightValue > setPointR && leftValue > setPointL){
						rigthMotor->run(FORWARD);
						if(72 + correctionL <= 255){
							rigthMotor->setSpeed(72 + correctionL);
						}
						else{
							rigthMotor->setSpeed(255);
						}
						leftMotor->run(BACKWARD);
						leftMotor->setSpeed(60);
					}

					//Up e Down alteraram Kp durante execução
					lcd_key = read_LCD_buttons();					
					if(lcd_key == 4){
						delay(300);
						lcd_key = 0;						
						Kp += 1;
					}
					if(lcd_key == 3){
						delay(300);
						lcd_key = 0;						
						Kp -= 1;
					}
					
					//Select encerra a rotina
					if(lcd_key == 1){
						delay(300);
						lcd_key = 0;						
						leftMotor->run(RELEASE);
						rigthMotor->run(RELEASE);
						lcd.clear();
						lcd.setCursor(0,0);
						lcd.print("Navegacao:");
						break;
					}
				}
			}

		}
	}

	delay(300);
	lcd_key = 0;

}


//Calibração dos sensores óptico-reflexivos
void LightSensorsCalibrate(){
	
	int i;
	
	lcd.clear();
	lcd.setCursor(0,0);
	lcd.print("Sensor Direita");
	lcd.setCursor(0,1);
	lcd.print("Branco:         ");
	while(lcd_key != 1){
		lcd_key = read_LCD_buttons();
	}
	if(lcd_key == 1){
		lcd_key = 0;
		rightValue = 0;
		for(i = 0; i < 10; i++){
			rightValue += analogRead(rightSensor);
		}
		whiteSurfaceR = rightValue/10;
		lcd.setCursor(8,1);
		lcd.print(whiteSurfaceR);
	}
	delay(1000);
	lcd.setCursor(0,1);
	lcd.print("Preto:          ");
	while(lcd_key != 1){
		lcd_key = read_LCD_buttons();
	}
	if(lcd_key == 1){
		lcd_key = 0;
		rightValue = 0;
		for(i = 0; i < 10; i++){
			rightValue += analogRead(rightSensor);
		}
		blackSurfaceR = rightValue/10;
		lcd.setCursor(8,1);
		lcd.print(blackSurfaceR);
	}
	delay(1000);
	
	lcd.clear();
	lcd.setCursor(0,0);
	lcd.print("Sensor Esquerda");
	lcd.setCursor(0,1);
	lcd.print("Branco:         ");
	while(lcd_key != 1){
		lcd_key = read_LCD_buttons();
	}
	if(lcd_key == 1){
		lcd_key = 0;
		leftValue = 0;
		for(i = 0; i < 10; i++){
			leftValue += analogRead(leftSensor);
		}
		whiteSurfaceL = leftValue/10;
		lcd.setCursor(8,1);
		lcd.print(whiteSurfaceL);
	}
	delay(1000);
	lcd.setCursor(0,1);
	lcd.print("Preto:          ");
	while(lcd_key != 1){
		lcd_key = read_LCD_buttons();
	}
	if(lcd_key == 1){
		lcd_key = 0;
		leftValue = 0;
		for(i = 0; i < 10; i++){
			leftValue += analogRead(leftSensor);
		}
		blackSurfaceL = leftValue/10;
		lcd.setCursor(8,1);
		lcd.print(blackSurfaceL);
	}
	delay(1000);
	
	lcd.clear();
	lcd.setCursor(0,0);
	lcd.print("CALIBRACAO");
	lcd.setCursor(0,1);
	lcd.print("PRONTA!");
	delay(500);

	lcd.clear();
	lcd.setCursor(0,0);
	lcd.print("Navegacao:");

}

// Trajeto em quadrado
void squarePath(){

  // Quantas voltas o encoder deve dar para atingir o angulo necessario
  angLeftMotor = 0.0611*ang;
  angRightMotor = 0.0667*ang;

  linePath();
  
  leftEncState = LOW;
  rightEncState = LOW;
  oldLeftEncState = LOW;
  oldRightEncState = LOW;
  leftEncCount = 0;
  rightEncCount = 0;
  getOutAngle = 0;
  
  angularPath();

  // Quantas voltas o encoder deve dar para atingir o angulo necessario
  angLeftMotor = 0.0611*ang;
  angRightMotor = 0.0667*ang;

  leftEncState = LOW;
  rightEncState = LOW;
  oldLeftEncState = LOW;
  oldRightEncState = LOW;
  leftEncCount = 0;
  rightEncCount = 0;
  getOutLine = 0;
  
  linePath();

  leftEncState = LOW;
  rightEncState = LOW;
  oldLeftEncState = LOW;
  oldRightEncState = LOW;
  leftEncCount = 0;
  rightEncCount = 0;
  getOutAngle = 0;
  
  angularPath();

  // Quantas voltas o encoder deve dar para atingir o angulo necessario
  angLeftMotor = 0.0611*ang;
  angRightMotor = 0.0667*ang;

  leftEncState = LOW;
  rightEncState = LOW;
  oldLeftEncState = LOW;
  oldRightEncState = LOW;
  leftEncCount = 0;
  rightEncCount = 0;
  getOutLine = 0;

  linePath();

  leftEncState = LOW;
  rightEncState = LOW;
  oldLeftEncState = LOW;
  oldRightEncState = LOW;
  leftEncCount = 0;
  rightEncCount = 0;
  getOutAngle = 0;
  
  angularPath();

  // Quantas voltas o encoder deve dar para atingir o angulo necessario
  angLeftMotor = 0.0611*ang;
  angRightMotor = 0.0667*ang;

  leftEncState = LOW;
  rightEncState = LOW;
  oldLeftEncState = LOW;
  oldRightEncState = LOW;
  leftEncCount = 0;
  rightEncCount = 0;
  getOutLine = 0;

  linePath();

}

// Trajeto em triangulo
void trianglePath(){

  // Quantas voltas o encoder deve dar para atingir o angulo necessario
  angLeftMotor = 0.0611*ang;
  angRightMotor = 0.0667*ang;

  linePath();
  
  leftEncState = LOW;
  rightEncState = LOW;
  oldLeftEncState = LOW;
  oldRightEncState = LOW;
  leftEncCount = 0;
  rightEncCount = 0;
  getOutAngle = 0;
  
  angularPath();

  // Quantas voltas o encoder deve dar para atingir o angulo necessario
  angLeftMotor = 0.0611*ang;
  angRightMotor = 0.0667*ang;

  leftEncState = LOW;
  rightEncState = LOW;
  oldLeftEncState = LOW;
  oldRightEncState = LOW;
  leftEncCount = 0;
  rightEncCount = 0;
  getOutLine = 0;
  
  linePath();

  leftEncState = LOW;
  rightEncState = LOW;
  oldLeftEncState = LOW;
  oldRightEncState = LOW;
  leftEncCount = 0;
  rightEncCount = 0;
  getOutAngle = 0;
  
  angularPath();

  // Quantas voltas o encoder deve dar para atingir o angulo necessario
  angLeftMotor = 0.0611*ang;
  angRightMotor = 0.0667*ang;

  leftEncState = LOW;
  rightEncState = LOW;
  oldLeftEncState = LOW;
  oldRightEncState = LOW;
  leftEncCount = 0;
  rightEncCount = 0;
  getOutLine = 0;

  linePath();

  /*leftEncState = LOW;
  rightEncState = LOW;
  oldLeftEncState = LOW;
  oldRightEncState = LOW;
  leftEncCount = 0;
  rightEncCount = 0;
  getOutAngle = 0;
  
  angularPath();*/

}

// Trajeto angular (virar para direita)
void angularPath(){

  while (getOutAngle != 1){
    
    // Condicao de parada
    // Verifica se os motores atingiram o num de voltas necessaria para o ang requisitado
    if((leftEncCount>=angLeftMotor)||(rightEncCount>=angRightMotor)){
      getOutAngle = 1;
    }

    if (getOutAngle == 1){
      leftMotor->run(RELEASE);
      rigthMotor->run(RELEASE);
      break;
    }
    
    // Gira o robo para a direita
    leftMotor->run(FORWARD);
    leftMotor->setSpeed(120);
    rigthMotor->run(BACKWARD);
    rigthMotor->setSpeed(142);

    // Faz a leitura dos encoderes
    leftEncState = digitalRead(leftEncoder);
    rightEncState = digitalRead(rigthEncoder);

    // Verifica borda de descida do encoder esquerdo
    if (leftEncState == HIGH){
      if (oldLeftEncState == LOW){
        leftEncCount++;
      }
    } else{ //Verifica borda de subida
      if (oldLeftEncState == HIGH){
        leftEncCount++;
      }
    }

    // Verifica borda de descida do encoder direito
    if (rightEncState == HIGH){
      if (oldRightEncState == LOW){
        rightEncCount++;
      }
    } else { //Verifica borda de subida
      if (oldRightEncState == HIGH){
        rightEncCount++;
      }
    }
        
    oldRightEncState = rightEncState;
    oldLeftEncState = leftEncState;
    
  }

}

// Trajeto em linha
void linePath(){
  
    while (getOutLine != 1){

      // Verifica se os motores atingiram o num de voltas necessaria para a dist requisitada
      if((leftEncCount>=distLeftMotor)&&(rightEncCount>=distRightMotor)){
        getOutLine = 1;
      }

      // Caso seja terminado
      if (getOutLine == 1){
        leftMotor->run(RELEASE);
        rigthMotor->run(RELEASE);
        break;
      }
      
      // Faz o movimento em linha reta
      leftMotor->run(FORWARD);
      leftMotor->setSpeed(120);
      rigthMotor->run(FORWARD);
      rigthMotor->setSpeed(145);
      
      // Faz a leitura dos encoderes
      leftEncState = digitalRead(leftEncoder);
      rightEncState = digitalRead(rigthEncoder);
  
      // Verifica borda de descida do encoder esquerdo
      if (leftEncState == HIGH){
        if (oldLeftEncState == LOW){
          leftEncCount++;
        }
      } else{ //Verifica borda de subida
        if (oldLeftEncState == HIGH){
          leftEncCount++;
        }
      }
  
      // Verifica borda de descida do encoder direito
      if (rightEncState == HIGH){
        if (oldRightEncState == LOW){
          rightEncCount++;
        }
      } else { //Verifica borda de subida
        if (oldRightEncState == HIGH){
          rightEncCount++;
        }
      }
      
      oldRightEncState = rightEncState;
      oldLeftEncState = leftEncState;
      
    }
  
}
