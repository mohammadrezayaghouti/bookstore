from rest_framework import serializers
from django.contrib.auth.hashers import make_password
from .models import *

class UserSerializer(serializers.ModelSerializer):
    class Meta:
        model = User
        fields = ['id', 'username', 'password', 'full_name', 'phone_number', 'birth_date', 'gender']
        extra_kwargs = {'password': {'write_only': True}}

    def create(self, validated_data):
        validated_data['password'] = make_password(validated_data['password'])
        return super().create(validated_data)

class BookSerializer(serializers.ModelSerializer):
    class Meta:
        model = Book
        fields = '__all__'
        read_only_fields = ['owner']

class CartItemSerializer(serializers.ModelSerializer):
    total_price = serializers.ReadOnlyField()
    book = BookSerializer(read_only=True)
    
    class Meta:
        model = CartItem
        fields = ['id', 'user', 'book', 'quantity', 'purchased', 'total_price']