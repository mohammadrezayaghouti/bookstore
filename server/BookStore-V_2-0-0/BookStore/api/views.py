from rest_framework import viewsets, permissions
from rest_framework.decorators import action
from rest_framework.response import Response
from rest_framework.views import APIView
from .serializers import *

class UserViewSet(viewsets.ModelViewSet):
    queryset = User.objects.all()
    serializer_class = UserSerializer

    def get_permissions(self):
        if self.action == 'create':  # Registration is open to all
            return [permissions.AllowAny()]
        elif self.action in ['retrieve', 'update', 'partial_update', 'destroy']:
            return [permissions.IsAuthenticated()]
        return super().get_permissions()

    def get_queryset(self):
        # Restrict access to the authenticated user's data only
        if self.request.user.is_authenticated:
            return User.objects.filter(id=self.request.user.id)
        return User.objects.none()

class UserDetailView(APIView):
    permission_classes = [permissions.IsAuthenticated]

    def get(self, request):
        serializer = UserSerializer(request.user)
        return Response(serializer.data)
    
    
class BookViewSet(viewsets.ModelViewSet):
    queryset = Book.objects.all()
    serializer_class = BookSerializer
    permission_classes = [permissions.IsAuthenticatedOrReadOnly]
    
    def perform_create(self, serializer):
        # Set the owner field to the current user during book creation
        serializer.save(owner=self.request.user)
    
    @action(detail=False, methods=['get'], permission_classes=[permissions.IsAuthenticated])
    def my_books(self, request):
        books = Book.objects.filter(owner=request.user)
        serializer = self.get_serializer(books, many=True)
        return Response(serializer.data)

class CartItemViewSet(viewsets.ModelViewSet):
    queryset = CartItem.objects.all()
    serializer_class = CartItemSerializer
    permission_classes = [permissions.IsAuthenticated]

    @action(detail=False, methods=['get'],  permission_classes=[permissions.IsAuthenticated])
    def my_cart(self, request):
        cart_items = self.queryset.filter(user=self.request.user).filter(purchased=False)
        serializer = self.get_serializer(cart_items, many=True)
        total = sum(item.total_price for item in cart_items)
        return Response({'items': serializer.data, 'total': total})

    @action(detail=False, methods=['get'],  permission_classes=[permissions.IsAuthenticated])
    def my_books(self, request):
        purchased_items = self.queryset.filter(user=self.request.user).filter(purchased=True)
        books = [item.book for item in purchased_items]
        serializer = BookSerializer(books, many=True)
        return Response(serializer.data)

    @action(detail=False, methods=['post'],  permission_classes=[permissions.IsAuthenticated])
    def purchase(self, request):
        cart_items = self.queryset.filter(user=self.request.user).filter(purchased=False)
        for item in cart_items:
            item.purchased = True
            item.save()
        return Response({'message': 'Purchase completed successfully!'})
    
    @action(detail=False, methods=['post'],  permission_classes=[permissions.IsAuthenticated])
    def add_to_cart(self, request):
        book_id = request.data.get('book_id')
        quantity = int(request.data.get('quantity', 1))

        try:
            book = Book.objects.get(id=book_id)
        except Book.DoesNotExist:
            return Response({'error': 'Book not found'}, status=404)

        cart_item, created = CartItem.objects.get_or_create(
            user=request.user, book=book, purchased=False,
            defaults={'quantity': quantity}
        )
        if not created:
            cart_item.quantity += quantity
            cart_item.save()

        return Response({'message': 'Book added to cart successfully!'})