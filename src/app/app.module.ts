import { NgModule } from '@angular/core';
import { BrowserModule } from '@angular/platform-browser';

import { AppRoutingModule } from './app-routing.module';
import { AppComponent } from './app.component';
import { initializeApp, provideFirebaseApp } from '@angular/fire/app';
import { getAuth, provideAuth } from '@angular/fire/auth';
import { getDatabase, provideDatabase } from '@angular/fire/database';

@NgModule({
  declarations: [
    AppComponent
  ],
  imports: [
    BrowserModule,
    AppRoutingModule
  ],
  providers: [
    provideFirebaseApp(() => initializeApp({"projectId":"smartparking-1ba32","appId":"1:942537240069:web:f13879d84af79c5255b66c","databaseURL":"https://smartparking-1ba32-default-rtdb.europe-west1.firebasedatabase.app","storageBucket":"smartparking-1ba32.firebasestorage.app","apiKey":"AIzaSyB1UzxVHzQtqLiazZ_0ySHneJyx2ZcxUzs","authDomain":"smartparking-1ba32.firebaseapp.com","messagingSenderId":"942537240069"})),
    provideAuth(() => getAuth()),
    provideDatabase(() => getDatabase())
  ],
  bootstrap: [AppComponent]
})
export class AppModule { }
