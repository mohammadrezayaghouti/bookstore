from django.contrib import admin
from .models import *

@admin.register(User)
class UserAdmin(admin.ModelAdmin):
    list_display = ('username', 'full_name', 'email', 'phone_number', 'birth_date', 'gender')
    search_fields = ('username', 'full_name', 'email', 'phone_number')
    list_filter = ('gender',)

@admin.register(Book)
class BookAdmin(admin.ModelAdmin):
    list_display = ('title', 'author', 'publisher', 'price', 'stock')
    search_fields = ('title', 'author', 'publisher')
    list_filter = ('publisher',)

@admin.register(CartItem)
class CartItemAdmin(admin.ModelAdmin):
    list_display = ('user', 'book', 'quantity', 'purchased', 'total_price')
    search_fields = ('user__username', 'book__title')
    list_filter = ('purchased',)
